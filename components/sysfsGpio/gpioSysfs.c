/**
 * @file gpioSysfs.c
 *
 * GPIO API implementation.
 * The GPIO API implementation for Sierra devices. Some of the features
 * of the generic API are not supported.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "gpioSysfs.h"
#include "watchdogChain.h"

//--------------------------------------------------------------------------------------------------
/**
 * Pin1 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 8

static struct gpioSysfs_Gpio SysfsGpioPin1 = {1,"gpio1",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin1 = &SysfsGpioPin1;

void gpioPin1_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin1, fd, events);
}

le_result_t le_gpioPin1_SetInput (le_gpioPin1_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin1, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin1_SetPushPullOutput (le_gpioPin1_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin1, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin1_SetOpenDrainOutput (le_gpioPin1_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin1, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin1_SetTriStateOutput (le_gpioPin1_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin1, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin1_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin1, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin1_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin1, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin1_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin1, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin1_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin1);
}

le_result_t le_gpioPin1_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin1);
}

bool le_gpioPin1_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin1);
}

le_result_t le_gpioPin1_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin1);
}

bool le_gpioPin1_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin1);
}

bool le_gpioPin1_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin1);
}

bool le_gpioPin1_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin1);
}

le_gpioPin1_Edge_t le_gpioPin1_GetEdgeSense (void)
{
    return (le_gpioPin1_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin1);
}

le_gpioPin1_Polarity_t le_gpioPin1_GetPolarity (void)
{
    return (le_gpioPin1_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin1);
}

le_gpioPin1_PullUpDown_t le_gpioPin1_GetPullUpDown (void)
{
    return (le_gpioPin1_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin1);
}

le_gpioPin1_ChangeEventHandlerRef_t le_gpioPin1_AddChangeEventHandler
(
    le_gpioPin1_Edge_t trigger,
    le_gpioPin1_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin1_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin1, gpioPin1_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin1_RemoveChangeEventHandler(le_gpioPin1_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin1, addHandlerRef);
}

le_result_t le_gpioPin1_SetEdgeSense (le_gpioPin1_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin1, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin1_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin1);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin1 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin2 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin2 = {2,"gpio2",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin2 = &SysfsGpioPin2;

void gpioPin2_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin2, fd, events);
}

le_result_t le_gpioPin2_SetInput (le_gpioPin2_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin2, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin2_SetPushPullOutput (le_gpioPin2_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin2, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin2_SetOpenDrainOutput (le_gpioPin2_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin2, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin2_SetTriStateOutput (le_gpioPin2_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin2, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin2_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin2, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin2_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin2, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin2_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin2, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin2_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin2);
}

le_result_t le_gpioPin2_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin2);
}

bool le_gpioPin2_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin2);
}

le_result_t le_gpioPin2_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin2);
}

bool le_gpioPin2_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin2);
}

bool le_gpioPin2_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin2);
}

bool le_gpioPin2_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin2);
}

le_gpioPin2_Edge_t le_gpioPin2_GetEdgeSense (void)
{
    return (le_gpioPin2_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin2);
}

le_gpioPin2_Polarity_t le_gpioPin2_GetPolarity (void)
{
    return (le_gpioPin2_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin2);
}

le_gpioPin2_PullUpDown_t le_gpioPin2_GetPullUpDown (void)
{
    return (le_gpioPin2_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin2);
}

le_gpioPin2_ChangeEventHandlerRef_t le_gpioPin2_AddChangeEventHandler
(
    le_gpioPin2_Edge_t trigger,
    le_gpioPin2_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin2_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin2, gpioPin2_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin2_RemoveChangeEventHandler(le_gpioPin2_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin2, addHandlerRef);
}

le_result_t le_gpioPin2_SetEdgeSense (le_gpioPin2_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin2, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin2_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin2);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin2 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin3 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin3 = {3,"gpio3",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin3 = &SysfsGpioPin3;

void gpioPin3_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin3, fd, events);
}

le_result_t le_gpioPin3_SetInput (le_gpioPin3_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin3, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin3_SetPushPullOutput (le_gpioPin3_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin3, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin3_SetOpenDrainOutput (le_gpioPin3_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin3, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin3_SetTriStateOutput (le_gpioPin3_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin3, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin3_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin3, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin3_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin3, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin3_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin3, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin3_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin3);
}

le_result_t le_gpioPin3_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin3);
}

bool le_gpioPin3_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin3);
}

le_result_t le_gpioPin3_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin3);
}

bool le_gpioPin3_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin3);
}

bool le_gpioPin3_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin3);
}

bool le_gpioPin3_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin3);
}

le_gpioPin3_Edge_t le_gpioPin3_GetEdgeSense (void)
{
    return (le_gpioPin3_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin3);
}

le_gpioPin3_Polarity_t le_gpioPin3_GetPolarity (void)
{
    return (le_gpioPin3_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin3);
}

le_gpioPin3_PullUpDown_t le_gpioPin3_GetPullUpDown (void)
{
    return (le_gpioPin3_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin3);
}

le_gpioPin3_ChangeEventHandlerRef_t le_gpioPin3_AddChangeEventHandler
(
    le_gpioPin3_Edge_t trigger,
    le_gpioPin3_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin3_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin3, gpioPin3_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin3_RemoveChangeEventHandler(le_gpioPin3_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin3, addHandlerRef);
}

le_result_t le_gpioPin3_SetEdgeSense (le_gpioPin3_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin3, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin3_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin3);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin3 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin4 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin4 = {4,"gpio4",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin4 = &SysfsGpioPin4;

void gpioPin4_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin4, fd, events);
}

le_result_t le_gpioPin4_SetInput (le_gpioPin4_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin4, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin4_SetPushPullOutput (le_gpioPin4_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin4, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin4_SetOpenDrainOutput (le_gpioPin4_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin4, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin4_SetTriStateOutput (le_gpioPin4_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin4, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin4_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin4, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin4_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin4, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin4_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin4, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin4_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin4);
}

le_result_t le_gpioPin4_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin4);
}

bool le_gpioPin4_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin4);
}

le_result_t le_gpioPin4_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin4);
}

bool le_gpioPin4_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin4);
}

bool le_gpioPin4_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin4);
}

bool le_gpioPin4_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin4);
}

le_gpioPin4_Edge_t le_gpioPin4_GetEdgeSense (void)
{
    return (le_gpioPin4_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin4);
}

le_gpioPin4_Polarity_t le_gpioPin4_GetPolarity (void)
{
    return (le_gpioPin4_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin4);
}

le_gpioPin4_PullUpDown_t le_gpioPin4_GetPullUpDown (void)
{
    return (le_gpioPin4_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin4);
}

le_gpioPin4_ChangeEventHandlerRef_t le_gpioPin4_AddChangeEventHandler
(
    le_gpioPin4_Edge_t trigger,
    le_gpioPin4_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin4_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin4, gpioPin4_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin4_RemoveChangeEventHandler(le_gpioPin4_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin4, addHandlerRef);
}

le_result_t le_gpioPin4_SetEdgeSense (le_gpioPin4_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin4, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin4_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin4);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin4 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin5 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin5 = {5,"gpio5",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin5 = &SysfsGpioPin5;

void gpioPin5_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin5, fd, events);
}

le_result_t le_gpioPin5_SetInput (le_gpioPin5_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin5, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin5_SetPushPullOutput (le_gpioPin5_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin5, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin5_SetOpenDrainOutput (le_gpioPin5_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin5, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin5_SetTriStateOutput (le_gpioPin5_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin5, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin5_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin5, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin5_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin5, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin5_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin5, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin5_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin5);
}

le_result_t le_gpioPin5_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin5);
}

bool le_gpioPin5_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin5);
}

le_result_t le_gpioPin5_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin5);
}

bool le_gpioPin5_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin5);
}

bool le_gpioPin5_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin5);
}

bool le_gpioPin5_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin5);
}

le_gpioPin5_Edge_t le_gpioPin5_GetEdgeSense (void)
{
    return (le_gpioPin5_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin5);
}

le_gpioPin5_Polarity_t le_gpioPin5_GetPolarity (void)
{
    return (le_gpioPin5_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin5);
}

le_gpioPin5_PullUpDown_t le_gpioPin5_GetPullUpDown (void)
{
    return (le_gpioPin5_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin5);
}

le_gpioPin5_ChangeEventHandlerRef_t le_gpioPin5_AddChangeEventHandler
(
    le_gpioPin5_Edge_t trigger,
    le_gpioPin5_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin5_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin5, gpioPin5_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin5_RemoveChangeEventHandler(le_gpioPin5_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin5, addHandlerRef);
}

le_result_t le_gpioPin5_SetEdgeSense (le_gpioPin5_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin5, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin5_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin5);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin5 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin6 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin6 = {6,"gpio6",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin6 = &SysfsGpioPin6;

void gpioPin6_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin6, fd, events);
}

le_result_t le_gpioPin6_SetInput (le_gpioPin6_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin6, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin6_SetPushPullOutput (le_gpioPin6_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin6, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin6_SetOpenDrainOutput (le_gpioPin6_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin6, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin6_SetTriStateOutput (le_gpioPin6_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin6, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin6_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin6, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin6_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin6, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin6_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin6, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin6_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin6);
}

le_result_t le_gpioPin6_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin6);
}

bool le_gpioPin6_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin6);
}

le_result_t le_gpioPin6_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin6);
}

bool le_gpioPin6_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin6);
}

bool le_gpioPin6_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin6);
}

bool le_gpioPin6_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin6);
}

le_gpioPin6_Edge_t le_gpioPin6_GetEdgeSense (void)
{
    return (le_gpioPin6_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin6);
}

le_gpioPin6_Polarity_t le_gpioPin6_GetPolarity (void)
{
    return (le_gpioPin6_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin6);
}

le_gpioPin6_PullUpDown_t le_gpioPin6_GetPullUpDown (void)
{
    return (le_gpioPin6_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin6);
}

le_gpioPin6_ChangeEventHandlerRef_t le_gpioPin6_AddChangeEventHandler
(
    le_gpioPin6_Edge_t trigger,
    le_gpioPin6_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin6_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin6, gpioPin6_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin6_RemoveChangeEventHandler(le_gpioPin6_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin6, addHandlerRef);
}

le_result_t le_gpioPin6_SetEdgeSense (le_gpioPin6_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin6, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin6_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin6);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin6 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin7 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin7 = {7,"gpio7",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin7 = &SysfsGpioPin7;

void gpioPin7_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin7, fd, events);
}

le_result_t le_gpioPin7_SetInput (le_gpioPin7_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin7, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin7_SetPushPullOutput (le_gpioPin7_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin7, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin7_SetOpenDrainOutput (le_gpioPin7_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin7, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin7_SetTriStateOutput (le_gpioPin7_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin7, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin7_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin7, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin7_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin7, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin7_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin7, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin7_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin7);
}

le_result_t le_gpioPin7_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin7);
}

bool le_gpioPin7_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin7);
}

le_result_t le_gpioPin7_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin7);
}

bool le_gpioPin7_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin7);
}

bool le_gpioPin7_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin7);
}

bool le_gpioPin7_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin7);
}

le_gpioPin7_Edge_t le_gpioPin7_GetEdgeSense (void)
{
    return (le_gpioPin7_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin7);
}

le_gpioPin7_Polarity_t le_gpioPin7_GetPolarity (void)
{
    return (le_gpioPin7_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin7);
}

le_gpioPin7_PullUpDown_t le_gpioPin7_GetPullUpDown (void)
{
    return (le_gpioPin7_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin7);
}

le_gpioPin7_ChangeEventHandlerRef_t le_gpioPin7_AddChangeEventHandler
(
    le_gpioPin7_Edge_t trigger,
    le_gpioPin7_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin7_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin7, gpioPin7_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin7_RemoveChangeEventHandler(le_gpioPin7_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin7, addHandlerRef);
}

le_result_t le_gpioPin7_SetEdgeSense (le_gpioPin7_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin7, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin7_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin7);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin7 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin8 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin8 = {8,"gpio8",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin8 = &SysfsGpioPin8;

void gpioPin8_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin8, fd, events);
}

le_result_t le_gpioPin8_SetInput (le_gpioPin8_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin8, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin8_SetPushPullOutput (le_gpioPin8_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin8, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin8_SetOpenDrainOutput (le_gpioPin8_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin8, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin8_SetTriStateOutput (le_gpioPin8_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin8, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin8_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin8, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin8_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin8, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin8_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin8, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin8_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin8);
}

le_result_t le_gpioPin8_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin8);
}

bool le_gpioPin8_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin8);
}

le_result_t le_gpioPin8_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin8);
}

bool le_gpioPin8_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin8);
}

bool le_gpioPin8_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin8);
}

bool le_gpioPin8_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin8);
}

le_gpioPin8_Edge_t le_gpioPin8_GetEdgeSense (void)
{
    return (le_gpioPin8_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin8);
}

le_gpioPin8_Polarity_t le_gpioPin8_GetPolarity (void)
{
    return (le_gpioPin8_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin8);
}

le_gpioPin8_PullUpDown_t le_gpioPin8_GetPullUpDown (void)
{
    return (le_gpioPin8_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin8);
}

le_gpioPin8_ChangeEventHandlerRef_t le_gpioPin8_AddChangeEventHandler
(
    le_gpioPin8_Edge_t trigger,
    le_gpioPin8_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin8_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin8, gpioPin8_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin8_RemoveChangeEventHandler(le_gpioPin8_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin8, addHandlerRef);
}

le_result_t le_gpioPin8_SetEdgeSense (le_gpioPin8_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin8, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin8_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin8);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin8 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin9 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin9 = {9,"gpio9",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin9 = &SysfsGpioPin9;

void gpioPin9_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin9, fd, events);
}

le_result_t le_gpioPin9_SetInput (le_gpioPin9_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin9, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin9_SetPushPullOutput (le_gpioPin9_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin9, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin9_SetOpenDrainOutput (le_gpioPin9_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin9, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin9_SetTriStateOutput (le_gpioPin9_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin9, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin9_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin9, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin9_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin9, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin9_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin9, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin9_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin9);
}

le_result_t le_gpioPin9_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin9);
}

bool le_gpioPin9_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin9);
}

le_result_t le_gpioPin9_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin9);
}

bool le_gpioPin9_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin9);
}

bool le_gpioPin9_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin9);
}

bool le_gpioPin9_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin9);
}

le_gpioPin9_Edge_t le_gpioPin9_GetEdgeSense (void)
{
    return (le_gpioPin9_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin9);
}

le_gpioPin9_Polarity_t le_gpioPin9_GetPolarity (void)
{
    return (le_gpioPin9_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin9);
}

le_gpioPin9_PullUpDown_t le_gpioPin9_GetPullUpDown (void)
{
    return (le_gpioPin9_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin9);
}

le_gpioPin9_ChangeEventHandlerRef_t le_gpioPin9_AddChangeEventHandler
(
    le_gpioPin9_Edge_t trigger,
    le_gpioPin9_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin9_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin9, gpioPin9_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin9_RemoveChangeEventHandler(le_gpioPin9_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin9, addHandlerRef);
}

le_result_t le_gpioPin9_SetEdgeSense (le_gpioPin9_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin9, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin9_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin9);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin9 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin10 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin10 = {10,"gpio10",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin10 = &SysfsGpioPin10;

void gpioPin10_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin10, fd, events);
}

le_result_t le_gpioPin10_SetInput (le_gpioPin10_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin10, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin10_SetPushPullOutput (le_gpioPin10_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin10, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin10_SetOpenDrainOutput (le_gpioPin10_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin10, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin10_SetTriStateOutput (le_gpioPin10_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin10, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin10_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin10, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin10_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin10, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin10_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin10, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin10_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin10);
}

le_result_t le_gpioPin10_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin10);
}

bool le_gpioPin10_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin10);
}

le_result_t le_gpioPin10_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin10);
}

bool le_gpioPin10_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin10);
}

bool le_gpioPin10_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin10);
}

bool le_gpioPin10_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin10);
}

le_gpioPin10_Edge_t le_gpioPin10_GetEdgeSense (void)
{
    return (le_gpioPin10_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin10);
}

le_gpioPin10_Polarity_t le_gpioPin10_GetPolarity (void)
{
    return (le_gpioPin10_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin10);
}

le_gpioPin10_PullUpDown_t le_gpioPin10_GetPullUpDown (void)
{
    return (le_gpioPin10_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin10);
}

le_gpioPin10_ChangeEventHandlerRef_t le_gpioPin10_AddChangeEventHandler
(
    le_gpioPin10_Edge_t trigger,
    le_gpioPin10_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin10_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin10, gpioPin10_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin10_RemoveChangeEventHandler(le_gpioPin10_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin10, addHandlerRef);
}

le_result_t le_gpioPin10_SetEdgeSense (le_gpioPin10_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin10, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin10_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin10);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin10 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin11 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin11 = {11,"gpio11",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin11 = &SysfsGpioPin11;

void gpioPin11_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin11, fd, events);
}

le_result_t le_gpioPin11_SetInput (le_gpioPin11_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin11, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin11_SetPushPullOutput (le_gpioPin11_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin11, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin11_SetOpenDrainOutput (le_gpioPin11_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin11, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin11_SetTriStateOutput (le_gpioPin11_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin11, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin11_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin11, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin11_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin11, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin11_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin11, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin11_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin11);
}

le_result_t le_gpioPin11_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin11);
}

bool le_gpioPin11_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin11);
}

le_result_t le_gpioPin11_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin11);
}

bool le_gpioPin11_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin11);
}

bool le_gpioPin11_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin11);
}

bool le_gpioPin11_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin11);
}

le_gpioPin11_Edge_t le_gpioPin11_GetEdgeSense (void)
{
    return (le_gpioPin11_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin11);
}

le_gpioPin11_Polarity_t le_gpioPin11_GetPolarity (void)
{
    return (le_gpioPin11_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin11);
}

le_gpioPin11_PullUpDown_t le_gpioPin11_GetPullUpDown (void)
{
    return (le_gpioPin11_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin11);
}

le_gpioPin11_ChangeEventHandlerRef_t le_gpioPin11_AddChangeEventHandler
(
    le_gpioPin11_Edge_t trigger,
    le_gpioPin11_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin11_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin11, gpioPin11_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin11_RemoveChangeEventHandler(le_gpioPin11_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin11, addHandlerRef);
}

le_result_t le_gpioPin11_SetEdgeSense (le_gpioPin11_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin11, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin11_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin11);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin11 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin12 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin12 = {12,"gpio12",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin12 = &SysfsGpioPin12;

void gpioPin12_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin12, fd, events);
}

le_result_t le_gpioPin12_SetInput (le_gpioPin12_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin12, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin12_SetPushPullOutput (le_gpioPin12_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin12, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin12_SetOpenDrainOutput (le_gpioPin12_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin12, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin12_SetTriStateOutput (le_gpioPin12_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin12, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin12_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin12, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin12_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin12, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin12_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin12, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin12_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin12);
}

le_result_t le_gpioPin12_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin12);
}

bool le_gpioPin12_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin12);
}

le_result_t le_gpioPin12_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin12);
}

bool le_gpioPin12_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin12);
}

bool le_gpioPin12_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin12);
}

bool le_gpioPin12_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin12);
}

le_gpioPin12_Edge_t le_gpioPin12_GetEdgeSense (void)
{
    return (le_gpioPin12_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin12);
}

le_gpioPin12_Polarity_t le_gpioPin12_GetPolarity (void)
{
    return (le_gpioPin12_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin12);
}

le_gpioPin12_PullUpDown_t le_gpioPin12_GetPullUpDown (void)
{
    return (le_gpioPin12_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin12);
}

le_gpioPin12_ChangeEventHandlerRef_t le_gpioPin12_AddChangeEventHandler
(
    le_gpioPin12_Edge_t trigger,
    le_gpioPin12_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin12_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin12, gpioPin12_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin12_RemoveChangeEventHandler(le_gpioPin12_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin12, addHandlerRef);
}

le_result_t le_gpioPin12_SetEdgeSense (le_gpioPin12_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin12, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin12_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin12);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin12 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin13 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin13 = {13,"gpio13",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin13 = &SysfsGpioPin13;

void gpioPin13_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin13, fd, events);
}

le_result_t le_gpioPin13_SetInput (le_gpioPin13_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin13, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin13_SetPushPullOutput (le_gpioPin13_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin13, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin13_SetOpenDrainOutput (le_gpioPin13_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin13, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin13_SetTriStateOutput (le_gpioPin13_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin13, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin13_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin13, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin13_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin13, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin13_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin13, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin13_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin13);
}

le_result_t le_gpioPin13_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin13);
}

bool le_gpioPin13_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin13);
}

le_result_t le_gpioPin13_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin13);
}

bool le_gpioPin13_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin13);
}

bool le_gpioPin13_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin13);
}

bool le_gpioPin13_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin13);
}

le_gpioPin13_Edge_t le_gpioPin13_GetEdgeSense (void)
{
    return (le_gpioPin13_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin13);
}

le_gpioPin13_Polarity_t le_gpioPin13_GetPolarity (void)
{
    return (le_gpioPin13_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin13);
}

le_gpioPin13_PullUpDown_t le_gpioPin13_GetPullUpDown (void)
{
    return (le_gpioPin13_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin13);
}

le_gpioPin13_ChangeEventHandlerRef_t le_gpioPin13_AddChangeEventHandler
(
    le_gpioPin13_Edge_t trigger,
    le_gpioPin13_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin13_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin13, gpioPin13_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin13_RemoveChangeEventHandler(le_gpioPin13_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin13, addHandlerRef);
}

le_result_t le_gpioPin13_SetEdgeSense (le_gpioPin13_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin13, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin13_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin13);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin13 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin14 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin14 = {14,"gpio14",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin14 = &SysfsGpioPin14;

void gpioPin14_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin14, fd, events);
}

le_result_t le_gpioPin14_SetInput (le_gpioPin14_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin14, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin14_SetPushPullOutput (le_gpioPin14_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin14, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin14_SetOpenDrainOutput (le_gpioPin14_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin14, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin14_SetTriStateOutput (le_gpioPin14_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin14, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin14_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin14, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin14_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin14, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin14_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin14, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin14_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin14);
}

le_result_t le_gpioPin14_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin14);
}

bool le_gpioPin14_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin14);
}

le_result_t le_gpioPin14_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin14);
}

bool le_gpioPin14_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin14);
}

bool le_gpioPin14_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin14);
}

bool le_gpioPin14_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin14);
}

le_gpioPin14_Edge_t le_gpioPin14_GetEdgeSense (void)
{
    return (le_gpioPin14_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin14);
}

le_gpioPin14_Polarity_t le_gpioPin14_GetPolarity (void)
{
    return (le_gpioPin14_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin14);
}

le_gpioPin14_PullUpDown_t le_gpioPin14_GetPullUpDown (void)
{
    return (le_gpioPin14_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin14);
}

le_gpioPin14_ChangeEventHandlerRef_t le_gpioPin14_AddChangeEventHandler
(
    le_gpioPin14_Edge_t trigger,
    le_gpioPin14_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin14_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin14, gpioPin14_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin14_RemoveChangeEventHandler(le_gpioPin14_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin14, addHandlerRef);
}

le_result_t le_gpioPin14_SetEdgeSense (le_gpioPin14_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin14, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin14_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin14);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin14 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin15 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin15 = {15,"gpio15",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin15 = &SysfsGpioPin15;

void gpioPin15_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin15, fd, events);
}

le_result_t le_gpioPin15_SetInput (le_gpioPin15_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin15, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin15_SetPushPullOutput (le_gpioPin15_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin15, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin15_SetOpenDrainOutput (le_gpioPin15_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin15, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin15_SetTriStateOutput (le_gpioPin15_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin15, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin15_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin15, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin15_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin15, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin15_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin15, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin15_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin15);
}

le_result_t le_gpioPin15_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin15);
}

bool le_gpioPin15_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin15);
}

le_result_t le_gpioPin15_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin15);
}

bool le_gpioPin15_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin15);
}

bool le_gpioPin15_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin15);
}

bool le_gpioPin15_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin15);
}

le_gpioPin15_Edge_t le_gpioPin15_GetEdgeSense (void)
{
    return (le_gpioPin15_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin15);
}

le_gpioPin15_Polarity_t le_gpioPin15_GetPolarity (void)
{
    return (le_gpioPin15_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin15);
}

le_gpioPin15_PullUpDown_t le_gpioPin15_GetPullUpDown (void)
{
    return (le_gpioPin15_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin15);
}

le_gpioPin15_ChangeEventHandlerRef_t le_gpioPin15_AddChangeEventHandler
(
    le_gpioPin15_Edge_t trigger,
    le_gpioPin15_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin15_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin15, gpioPin15_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin15_RemoveChangeEventHandler(le_gpioPin15_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin15, addHandlerRef);
}

le_result_t le_gpioPin15_SetEdgeSense (le_gpioPin15_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin15, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin15_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin15);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin15 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin16 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin16 = {16,"gpio16",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin16 = &SysfsGpioPin16;

void gpioPin16_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin16, fd, events);
}

le_result_t le_gpioPin16_SetInput (le_gpioPin16_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin16, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin16_SetPushPullOutput (le_gpioPin16_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin16, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin16_SetOpenDrainOutput (le_gpioPin16_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin16, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin16_SetTriStateOutput (le_gpioPin16_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin16, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin16_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin16, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin16_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin16, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin16_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin16, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin16_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin16);
}

le_result_t le_gpioPin16_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin16);
}

bool le_gpioPin16_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin16);
}

le_result_t le_gpioPin16_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin16);
}

bool le_gpioPin16_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin16);
}

bool le_gpioPin16_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin16);
}

bool le_gpioPin16_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin16);
}

le_gpioPin16_Edge_t le_gpioPin16_GetEdgeSense (void)
{
    return (le_gpioPin16_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin16);
}

le_gpioPin16_Polarity_t le_gpioPin16_GetPolarity (void)
{
    return (le_gpioPin16_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin16);
}

le_gpioPin16_PullUpDown_t le_gpioPin16_GetPullUpDown (void)
{
    return (le_gpioPin16_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin16);
}

le_gpioPin16_ChangeEventHandlerRef_t le_gpioPin16_AddChangeEventHandler
(
    le_gpioPin16_Edge_t trigger,
    le_gpioPin16_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin16_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin16, gpioPin16_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin16_RemoveChangeEventHandler(le_gpioPin16_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin16, addHandlerRef);
}

le_result_t le_gpioPin16_SetEdgeSense (le_gpioPin16_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin16, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin16_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin16);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin16 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin17 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin17 = {17,"gpio17",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin17 = &SysfsGpioPin17;

void gpioPin17_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin17, fd, events);
}

le_result_t le_gpioPin17_SetInput (le_gpioPin17_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin17, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin17_SetPushPullOutput (le_gpioPin17_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin17, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin17_SetOpenDrainOutput (le_gpioPin17_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin17, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin17_SetTriStateOutput (le_gpioPin17_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin17, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin17_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin17, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin17_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin17, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin17_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin17, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin17_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin17);
}

le_result_t le_gpioPin17_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin17);
}

bool le_gpioPin17_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin17);
}

le_result_t le_gpioPin17_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin17);
}

bool le_gpioPin17_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin17);
}

bool le_gpioPin17_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin17);
}

bool le_gpioPin17_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin17);
}

le_gpioPin17_Edge_t le_gpioPin17_GetEdgeSense (void)
{
    return (le_gpioPin17_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin17);
}

le_gpioPin17_Polarity_t le_gpioPin17_GetPolarity (void)
{
    return (le_gpioPin17_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin17);
}

le_gpioPin17_PullUpDown_t le_gpioPin17_GetPullUpDown (void)
{
    return (le_gpioPin17_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin17);
}

le_gpioPin17_ChangeEventHandlerRef_t le_gpioPin17_AddChangeEventHandler
(
    le_gpioPin17_Edge_t trigger,
    le_gpioPin17_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin17_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin17, gpioPin17_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin17_RemoveChangeEventHandler(le_gpioPin17_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin17, addHandlerRef);
}

le_result_t le_gpioPin17_SetEdgeSense (le_gpioPin17_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin17, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin17_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin17);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin17 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin18 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin18 = {18,"gpio18",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin18 = &SysfsGpioPin18;

void gpioPin18_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin18, fd, events);
}

le_result_t le_gpioPin18_SetInput (le_gpioPin18_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin18, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin18_SetPushPullOutput (le_gpioPin18_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin18, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin18_SetOpenDrainOutput (le_gpioPin18_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin18, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin18_SetTriStateOutput (le_gpioPin18_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin18, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin18_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin18, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin18_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin18, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin18_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin18, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin18_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin18);
}

le_result_t le_gpioPin18_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin18);
}

bool le_gpioPin18_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin18);
}

le_result_t le_gpioPin18_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin18);
}

bool le_gpioPin18_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin18);
}

bool le_gpioPin18_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin18);
}

bool le_gpioPin18_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin18);
}

le_gpioPin18_Edge_t le_gpioPin18_GetEdgeSense (void)
{
    return (le_gpioPin18_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin18);
}

le_gpioPin18_Polarity_t le_gpioPin18_GetPolarity (void)
{
    return (le_gpioPin18_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin18);
}

le_gpioPin18_PullUpDown_t le_gpioPin18_GetPullUpDown (void)
{
    return (le_gpioPin18_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin18);
}

le_gpioPin18_ChangeEventHandlerRef_t le_gpioPin18_AddChangeEventHandler
(
    le_gpioPin18_Edge_t trigger,
    le_gpioPin18_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin18_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin18, gpioPin18_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin18_RemoveChangeEventHandler(le_gpioPin18_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin18, addHandlerRef);
}

le_result_t le_gpioPin18_SetEdgeSense (le_gpioPin18_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin18, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin18_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin18);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin18 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin19 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin19 = {19,"gpio19",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin19 = &SysfsGpioPin19;

void gpioPin19_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin19, fd, events);
}

le_result_t le_gpioPin19_SetInput (le_gpioPin19_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin19, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin19_SetPushPullOutput (le_gpioPin19_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin19, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin19_SetOpenDrainOutput (le_gpioPin19_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin19, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin19_SetTriStateOutput (le_gpioPin19_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin19, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin19_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin19, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin19_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin19, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin19_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin19, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin19_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin19);
}

le_result_t le_gpioPin19_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin19);
}

bool le_gpioPin19_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin19);
}

le_result_t le_gpioPin19_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin19);
}

bool le_gpioPin19_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin19);
}

bool le_gpioPin19_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin19);
}

bool le_gpioPin19_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin19);
}

le_gpioPin19_Edge_t le_gpioPin19_GetEdgeSense (void)
{
    return (le_gpioPin19_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin19);
}

le_gpioPin19_Polarity_t le_gpioPin19_GetPolarity (void)
{
    return (le_gpioPin19_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin19);
}

le_gpioPin19_PullUpDown_t le_gpioPin19_GetPullUpDown (void)
{
    return (le_gpioPin19_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin19);
}

le_gpioPin19_ChangeEventHandlerRef_t le_gpioPin19_AddChangeEventHandler
(
    le_gpioPin19_Edge_t trigger,
    le_gpioPin19_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin19_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin19, gpioPin19_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin19_RemoveChangeEventHandler(le_gpioPin19_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin19, addHandlerRef);
}

le_result_t le_gpioPin19_SetEdgeSense (le_gpioPin19_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin19, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin19_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin19);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin19 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin20 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin20 = {20,"gpio20",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin20 = &SysfsGpioPin20;

void gpioPin20_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin20, fd, events);
}

le_result_t le_gpioPin20_SetInput (le_gpioPin20_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin20, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin20_SetPushPullOutput (le_gpioPin20_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin20, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin20_SetOpenDrainOutput (le_gpioPin20_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin20, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin20_SetTriStateOutput (le_gpioPin20_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin20, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin20_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin20, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin20_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin20, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin20_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin20, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin20_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin20);
}

le_result_t le_gpioPin20_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin20);
}

bool le_gpioPin20_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin20);
}

le_result_t le_gpioPin20_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin20);
}

bool le_gpioPin20_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin20);
}

bool le_gpioPin20_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin20);
}

bool le_gpioPin20_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin20);
}

le_gpioPin20_Edge_t le_gpioPin20_GetEdgeSense (void)
{
    return (le_gpioPin20_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin20);
}

le_gpioPin20_Polarity_t le_gpioPin20_GetPolarity (void)
{
    return (le_gpioPin20_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin20);
}

le_gpioPin20_PullUpDown_t le_gpioPin20_GetPullUpDown (void)
{
    return (le_gpioPin20_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin20);
}

le_gpioPin20_ChangeEventHandlerRef_t le_gpioPin20_AddChangeEventHandler
(
    le_gpioPin20_Edge_t trigger,
    le_gpioPin20_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin20_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin20, gpioPin20_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin20_RemoveChangeEventHandler(le_gpioPin20_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin20, addHandlerRef);
}

le_result_t le_gpioPin20_SetEdgeSense (le_gpioPin20_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin20, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin20_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin20);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin20 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin21 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin21 = {21,"gpio21",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin21 = &SysfsGpioPin21;

void gpioPin21_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin21, fd, events);
}

le_result_t le_gpioPin21_SetInput (le_gpioPin21_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin21, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin21_SetPushPullOutput (le_gpioPin21_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin21, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin21_SetOpenDrainOutput (le_gpioPin21_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin21, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin21_SetTriStateOutput (le_gpioPin21_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin21, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin21_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin21, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin21_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin21, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin21_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin21, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin21_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin21);
}

le_result_t le_gpioPin21_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin21);
}

bool le_gpioPin21_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin21);
}

le_result_t le_gpioPin21_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin21);
}

bool le_gpioPin21_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin21);
}

bool le_gpioPin21_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin21);
}

bool le_gpioPin21_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin21);
}

le_gpioPin21_Edge_t le_gpioPin21_GetEdgeSense (void)
{
    return (le_gpioPin21_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin21);
}

le_gpioPin21_Polarity_t le_gpioPin21_GetPolarity (void)
{
    return (le_gpioPin21_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin21);
}

le_gpioPin21_PullUpDown_t le_gpioPin21_GetPullUpDown (void)
{
    return (le_gpioPin21_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin21);
}

le_gpioPin21_ChangeEventHandlerRef_t le_gpioPin21_AddChangeEventHandler
(
    le_gpioPin21_Edge_t trigger,
    le_gpioPin21_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin21_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin21, gpioPin21_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin21_RemoveChangeEventHandler(le_gpioPin21_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin21, addHandlerRef);
}

le_result_t le_gpioPin21_SetEdgeSense (le_gpioPin21_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin21, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin21_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin21);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin21 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin22 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin22 = {22,"gpio22",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin22 = &SysfsGpioPin22;

void gpioPin22_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin22, fd, events);
}

le_result_t le_gpioPin22_SetInput (le_gpioPin22_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin22, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin22_SetPushPullOutput (le_gpioPin22_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin22, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin22_SetOpenDrainOutput (le_gpioPin22_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin22, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin22_SetTriStateOutput (le_gpioPin22_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin22, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin22_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin22, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin22_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin22, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin22_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin22, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin22_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin22);
}

le_result_t le_gpioPin22_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin22);
}

bool le_gpioPin22_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin22);
}

le_result_t le_gpioPin22_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin22);
}

bool le_gpioPin22_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin22);
}

bool le_gpioPin22_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin22);
}

bool le_gpioPin22_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin22);
}

le_gpioPin22_Edge_t le_gpioPin22_GetEdgeSense (void)
{
    return (le_gpioPin22_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin22);
}

le_gpioPin22_Polarity_t le_gpioPin22_GetPolarity (void)
{
    return (le_gpioPin22_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin22);
}

le_gpioPin22_PullUpDown_t le_gpioPin22_GetPullUpDown (void)
{
    return (le_gpioPin22_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin22);
}

le_gpioPin22_ChangeEventHandlerRef_t le_gpioPin22_AddChangeEventHandler
(
    le_gpioPin22_Edge_t trigger,
    le_gpioPin22_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin22_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin22, gpioPin22_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin22_RemoveChangeEventHandler(le_gpioPin22_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin22, addHandlerRef);
}

le_result_t le_gpioPin22_SetEdgeSense (le_gpioPin22_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin22, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin22_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin22);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin22 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin23 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin23 = {23,"gpio23",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin23 = &SysfsGpioPin23;

void gpioPin23_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin23, fd, events);
}

le_result_t le_gpioPin23_SetInput (le_gpioPin23_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin23, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin23_SetPushPullOutput (le_gpioPin23_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin23, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin23_SetOpenDrainOutput (le_gpioPin23_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin23, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin23_SetTriStateOutput (le_gpioPin23_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin23, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin23_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin23, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin23_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin23, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin23_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin23, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin23_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin23);
}

le_result_t le_gpioPin23_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin23);
}

bool le_gpioPin23_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin23);
}

le_result_t le_gpioPin23_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin23);
}

bool le_gpioPin23_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin23);
}

bool le_gpioPin23_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin23);
}

bool le_gpioPin23_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin23);
}

le_gpioPin23_Edge_t le_gpioPin23_GetEdgeSense (void)
{
    return (le_gpioPin23_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin23);
}

le_gpioPin23_Polarity_t le_gpioPin23_GetPolarity (void)
{
    return (le_gpioPin23_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin23);
}

le_gpioPin23_PullUpDown_t le_gpioPin23_GetPullUpDown (void)
{
    return (le_gpioPin23_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin23);
}

le_gpioPin23_ChangeEventHandlerRef_t le_gpioPin23_AddChangeEventHandler
(
    le_gpioPin23_Edge_t trigger,
    le_gpioPin23_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin23_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin23, gpioPin23_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin23_RemoveChangeEventHandler(le_gpioPin23_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin23, addHandlerRef);
}

le_result_t le_gpioPin23_SetEdgeSense (le_gpioPin23_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin23, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin23_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin23);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin23 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin24 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin24 = {24,"gpio24",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin24 = &SysfsGpioPin24;

void gpioPin24_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin24, fd, events);
}

le_result_t le_gpioPin24_SetInput (le_gpioPin24_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin24, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin24_SetPushPullOutput (le_gpioPin24_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin24, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin24_SetOpenDrainOutput (le_gpioPin24_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin24, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin24_SetTriStateOutput (le_gpioPin24_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin24, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin24_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin24, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin24_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin24, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin24_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin24, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin24_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin24);
}

le_result_t le_gpioPin24_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin24);
}

bool le_gpioPin24_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin24);
}

le_result_t le_gpioPin24_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin24);
}

bool le_gpioPin24_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin24);
}

bool le_gpioPin24_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin24);
}

bool le_gpioPin24_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin24);
}

le_gpioPin24_Edge_t le_gpioPin24_GetEdgeSense (void)
{
    return (le_gpioPin24_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin24);
}

le_gpioPin24_Polarity_t le_gpioPin24_GetPolarity (void)
{
    return (le_gpioPin24_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin24);
}

le_gpioPin24_PullUpDown_t le_gpioPin24_GetPullUpDown (void)
{
    return (le_gpioPin24_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin24);
}

le_gpioPin24_ChangeEventHandlerRef_t le_gpioPin24_AddChangeEventHandler
(
    le_gpioPin24_Edge_t trigger,
    le_gpioPin24_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin24_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin24, gpioPin24_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin24_RemoveChangeEventHandler(le_gpioPin24_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin24, addHandlerRef);
}

le_result_t le_gpioPin24_SetEdgeSense (le_gpioPin24_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin24, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin24_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin24);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin24 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin25 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin25 = {25,"gpio25",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin25 = &SysfsGpioPin25;

void gpioPin25_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin25, fd, events);
}

le_result_t le_gpioPin25_SetInput (le_gpioPin25_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin25, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin25_SetPushPullOutput (le_gpioPin25_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin25, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin25_SetOpenDrainOutput (le_gpioPin25_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin25, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin25_SetTriStateOutput (le_gpioPin25_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin25, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin25_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin25, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin25_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin25, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin25_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin25, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin25_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin25);
}

le_result_t le_gpioPin25_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin25);
}

bool le_gpioPin25_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin25);
}

le_result_t le_gpioPin25_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin25);
}

bool le_gpioPin25_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin25);
}

bool le_gpioPin25_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin25);
}

bool le_gpioPin25_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin25);
}

le_gpioPin25_Edge_t le_gpioPin25_GetEdgeSense (void)
{
    return (le_gpioPin25_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin25);
}

le_gpioPin25_Polarity_t le_gpioPin25_GetPolarity (void)
{
    return (le_gpioPin25_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin25);
}

le_gpioPin25_PullUpDown_t le_gpioPin25_GetPullUpDown (void)
{
    return (le_gpioPin25_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin25);
}

le_gpioPin25_ChangeEventHandlerRef_t le_gpioPin25_AddChangeEventHandler
(
    le_gpioPin25_Edge_t trigger,
    le_gpioPin25_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin25_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin25, gpioPin25_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin25_RemoveChangeEventHandler(le_gpioPin25_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin25, addHandlerRef);
}

le_result_t le_gpioPin25_SetEdgeSense (le_gpioPin25_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin25, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin25_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin25);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin25 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin26 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin26 = {26,"gpio26",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin26 = &SysfsGpioPin26;

void gpioPin26_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin26, fd, events);
}

le_result_t le_gpioPin26_SetInput (le_gpioPin26_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin26, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin26_SetPushPullOutput (le_gpioPin26_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin26, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin26_SetOpenDrainOutput (le_gpioPin26_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin26, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin26_SetTriStateOutput (le_gpioPin26_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin26, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin26_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin26, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin26_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin26, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin26_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin26, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin26_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin26);
}

le_result_t le_gpioPin26_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin26);
}

bool le_gpioPin26_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin26);
}

le_result_t le_gpioPin26_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin26);
}

bool le_gpioPin26_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin26);
}

bool le_gpioPin26_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin26);
}

bool le_gpioPin26_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin26);
}

le_gpioPin26_Edge_t le_gpioPin26_GetEdgeSense (void)
{
    return (le_gpioPin26_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin26);
}

le_gpioPin26_Polarity_t le_gpioPin26_GetPolarity (void)
{
    return (le_gpioPin26_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin26);
}

le_gpioPin26_PullUpDown_t le_gpioPin26_GetPullUpDown (void)
{
    return (le_gpioPin26_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin26);
}

le_gpioPin26_ChangeEventHandlerRef_t le_gpioPin26_AddChangeEventHandler
(
    le_gpioPin26_Edge_t trigger,
    le_gpioPin26_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin26_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin26, gpioPin26_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin26_RemoveChangeEventHandler(le_gpioPin26_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin26, addHandlerRef);
}

le_result_t le_gpioPin26_SetEdgeSense (le_gpioPin26_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin26, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin26_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin26);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin26 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin27 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin27 = {27,"gpio27",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin27 = &SysfsGpioPin27;

void gpioPin27_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin27, fd, events);
}

le_result_t le_gpioPin27_SetInput (le_gpioPin27_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin27, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin27_SetPushPullOutput (le_gpioPin27_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin27, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin27_SetOpenDrainOutput (le_gpioPin27_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin27, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin27_SetTriStateOutput (le_gpioPin27_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin27, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin27_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin27, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin27_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin27, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin27_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin27, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin27_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin27);
}

le_result_t le_gpioPin27_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin27);
}

bool le_gpioPin27_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin27);
}

le_result_t le_gpioPin27_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin27);
}

bool le_gpioPin27_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin27);
}

bool le_gpioPin27_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin27);
}

bool le_gpioPin27_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin27);
}

le_gpioPin27_Edge_t le_gpioPin27_GetEdgeSense (void)
{
    return (le_gpioPin27_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin27);
}

le_gpioPin27_Polarity_t le_gpioPin27_GetPolarity (void)
{
    return (le_gpioPin27_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin27);
}

le_gpioPin27_PullUpDown_t le_gpioPin27_GetPullUpDown (void)
{
    return (le_gpioPin27_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin27);
}

le_gpioPin27_ChangeEventHandlerRef_t le_gpioPin27_AddChangeEventHandler
(
    le_gpioPin27_Edge_t trigger,
    le_gpioPin27_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin27_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin27, gpioPin27_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin27_RemoveChangeEventHandler(le_gpioPin27_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin27, addHandlerRef);
}

le_result_t le_gpioPin27_SetEdgeSense (le_gpioPin27_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin27, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin27_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin27);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin27 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin28 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin28 = {28,"gpio28",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin28 = &SysfsGpioPin28;

void gpioPin28_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin28, fd, events);
}

le_result_t le_gpioPin28_SetInput (le_gpioPin28_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin28, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin28_SetPushPullOutput (le_gpioPin28_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin28, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin28_SetOpenDrainOutput (le_gpioPin28_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin28, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin28_SetTriStateOutput (le_gpioPin28_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin28, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin28_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin28, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin28_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin28, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin28_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin28, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin28_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin28);
}

le_result_t le_gpioPin28_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin28);
}

bool le_gpioPin28_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin28);
}

le_result_t le_gpioPin28_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin28);
}

bool le_gpioPin28_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin28);
}

bool le_gpioPin28_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin28);
}

bool le_gpioPin28_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin28);
}

le_gpioPin28_Edge_t le_gpioPin28_GetEdgeSense (void)
{
    return (le_gpioPin28_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin28);
}

le_gpioPin28_Polarity_t le_gpioPin28_GetPolarity (void)
{
    return (le_gpioPin28_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin28);
}

le_gpioPin28_PullUpDown_t le_gpioPin28_GetPullUpDown (void)
{
    return (le_gpioPin28_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin28);
}

le_gpioPin28_ChangeEventHandlerRef_t le_gpioPin28_AddChangeEventHandler
(
    le_gpioPin28_Edge_t trigger,
    le_gpioPin28_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin28_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin28, gpioPin28_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin28_RemoveChangeEventHandler(le_gpioPin28_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin28, addHandlerRef);
}

le_result_t le_gpioPin28_SetEdgeSense (le_gpioPin28_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin28, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin28_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin28);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin28 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin29 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin29 = {29,"gpio29",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin29 = &SysfsGpioPin29;

void gpioPin29_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin29, fd, events);
}

le_result_t le_gpioPin29_SetInput (le_gpioPin29_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin29, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin29_SetPushPullOutput (le_gpioPin29_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin29, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin29_SetOpenDrainOutput (le_gpioPin29_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin29, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin29_SetTriStateOutput (le_gpioPin29_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin29, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin29_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin29, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin29_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin29, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin29_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin29, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin29_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin29);
}

le_result_t le_gpioPin29_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin29);
}

bool le_gpioPin29_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin29);
}

le_result_t le_gpioPin29_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin29);
}

bool le_gpioPin29_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin29);
}

bool le_gpioPin29_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin29);
}

bool le_gpioPin29_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin29);
}

le_gpioPin29_Edge_t le_gpioPin29_GetEdgeSense (void)
{
    return (le_gpioPin29_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin29);
}

le_gpioPin29_Polarity_t le_gpioPin29_GetPolarity (void)
{
    return (le_gpioPin29_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin29);
}

le_gpioPin29_PullUpDown_t le_gpioPin29_GetPullUpDown (void)
{
    return (le_gpioPin29_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin29);
}

le_gpioPin29_ChangeEventHandlerRef_t le_gpioPin29_AddChangeEventHandler
(
    le_gpioPin29_Edge_t trigger,
    le_gpioPin29_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin29_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin29, gpioPin29_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin29_RemoveChangeEventHandler(le_gpioPin29_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin29, addHandlerRef);
}

le_result_t le_gpioPin29_SetEdgeSense (le_gpioPin29_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin29, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin29_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin29);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin29 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin30 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin30 = {30,"gpio30",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin30 = &SysfsGpioPin30;

void gpioPin30_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin30, fd, events);
}

le_result_t le_gpioPin30_SetInput (le_gpioPin30_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin30, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin30_SetPushPullOutput (le_gpioPin30_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin30, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin30_SetOpenDrainOutput (le_gpioPin30_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin30, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin30_SetTriStateOutput (le_gpioPin30_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin30, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin30_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin30, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin30_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin30, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin30_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin30, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin30_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin30);
}

le_result_t le_gpioPin30_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin30);
}

bool le_gpioPin30_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin30);
}

le_result_t le_gpioPin30_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin30);
}

bool le_gpioPin30_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin30);
}

bool le_gpioPin30_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin30);
}

bool le_gpioPin30_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin30);
}

le_gpioPin30_Edge_t le_gpioPin30_GetEdgeSense (void)
{
    return (le_gpioPin30_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin30);
}

le_gpioPin30_Polarity_t le_gpioPin30_GetPolarity (void)
{
    return (le_gpioPin30_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin30);
}

le_gpioPin30_PullUpDown_t le_gpioPin30_GetPullUpDown (void)
{
    return (le_gpioPin30_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin30);
}

le_gpioPin30_ChangeEventHandlerRef_t le_gpioPin30_AddChangeEventHandler
(
    le_gpioPin30_Edge_t trigger,
    le_gpioPin30_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin30_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin30, gpioPin30_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin30_RemoveChangeEventHandler(le_gpioPin30_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin30, addHandlerRef);
}

le_result_t le_gpioPin30_SetEdgeSense (le_gpioPin30_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin30, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin30_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin30);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin30 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin31 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin31 = {31,"gpio31",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin31 = &SysfsGpioPin31;

void gpioPin31_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin31, fd, events);
}

le_result_t le_gpioPin31_SetInput (le_gpioPin31_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin31, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin31_SetPushPullOutput (le_gpioPin31_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin31, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin31_SetOpenDrainOutput (le_gpioPin31_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin31, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin31_SetTriStateOutput (le_gpioPin31_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin31, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin31_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin31, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin31_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin31, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin31_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin31, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin31_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin31);
}

le_result_t le_gpioPin31_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin31);
}

bool le_gpioPin31_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin31);
}

le_result_t le_gpioPin31_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin31);
}

bool le_gpioPin31_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin31);
}

bool le_gpioPin31_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin31);
}

bool le_gpioPin31_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin31);
}

le_gpioPin31_Edge_t le_gpioPin31_GetEdgeSense (void)
{
    return (le_gpioPin31_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin31);
}

le_gpioPin31_Polarity_t le_gpioPin31_GetPolarity (void)
{
    return (le_gpioPin31_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin31);
}

le_gpioPin31_PullUpDown_t le_gpioPin31_GetPullUpDown (void)
{
    return (le_gpioPin31_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin31);
}

le_gpioPin31_ChangeEventHandlerRef_t le_gpioPin31_AddChangeEventHandler
(
    le_gpioPin31_Edge_t trigger,
    le_gpioPin31_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin31_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin31, gpioPin31_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin31_RemoveChangeEventHandler(le_gpioPin31_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin31, addHandlerRef);
}

le_result_t le_gpioPin31_SetEdgeSense (le_gpioPin31_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin31, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin31_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin31);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin31 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin32 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin32 = {32,"gpio32",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin32 = &SysfsGpioPin32;

void gpioPin32_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin32, fd, events);
}

le_result_t le_gpioPin32_SetInput (le_gpioPin32_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin32, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin32_SetPushPullOutput (le_gpioPin32_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin32, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin32_SetOpenDrainOutput (le_gpioPin32_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin32, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin32_SetTriStateOutput (le_gpioPin32_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin32, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin32_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin32, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin32_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin32, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin32_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin32, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin32_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin32);
}

le_result_t le_gpioPin32_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin32);
}

bool le_gpioPin32_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin32);
}

le_result_t le_gpioPin32_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin32);
}

bool le_gpioPin32_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin32);
}

bool le_gpioPin32_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin32);
}

bool le_gpioPin32_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin32);
}

le_gpioPin32_Edge_t le_gpioPin32_GetEdgeSense (void)
{
    return (le_gpioPin32_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin32);
}

le_gpioPin32_Polarity_t le_gpioPin32_GetPolarity (void)
{
    return (le_gpioPin32_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin32);
}

le_gpioPin32_PullUpDown_t le_gpioPin32_GetPullUpDown (void)
{
    return (le_gpioPin32_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin32);
}

le_gpioPin32_ChangeEventHandlerRef_t le_gpioPin32_AddChangeEventHandler
(
    le_gpioPin32_Edge_t trigger,
    le_gpioPin32_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin32_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin32, gpioPin32_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin32_RemoveChangeEventHandler(le_gpioPin32_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin32, addHandlerRef);
}

le_result_t le_gpioPin32_SetEdgeSense (le_gpioPin32_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin32, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin32_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin32);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin32 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin33 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin33 = {33,"gpio33",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin33 = &SysfsGpioPin33;

void gpioPin33_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin33, fd, events);
}

le_result_t le_gpioPin33_SetInput (le_gpioPin33_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin33, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin33_SetPushPullOutput (le_gpioPin33_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin33, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin33_SetOpenDrainOutput (le_gpioPin33_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin33, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin33_SetTriStateOutput (le_gpioPin33_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin33, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin33_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin33, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin33_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin33, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin33_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin33, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin33_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin33);
}

le_result_t le_gpioPin33_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin33);
}

bool le_gpioPin33_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin33);
}

le_result_t le_gpioPin33_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin33);
}

bool le_gpioPin33_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin33);
}

bool le_gpioPin33_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin33);
}

bool le_gpioPin33_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin33);
}

le_gpioPin33_Edge_t le_gpioPin33_GetEdgeSense (void)
{
    return (le_gpioPin33_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin33);
}

le_gpioPin33_Polarity_t le_gpioPin33_GetPolarity (void)
{
    return (le_gpioPin33_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin33);
}

le_gpioPin33_PullUpDown_t le_gpioPin33_GetPullUpDown (void)
{
    return (le_gpioPin33_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin33);
}

le_gpioPin33_ChangeEventHandlerRef_t le_gpioPin33_AddChangeEventHandler
(
    le_gpioPin33_Edge_t trigger,
    le_gpioPin33_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin33_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin33, gpioPin33_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin33_RemoveChangeEventHandler(le_gpioPin33_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin33, addHandlerRef);
}

le_result_t le_gpioPin33_SetEdgeSense (le_gpioPin33_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin33, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin33_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin33);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin33 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin34 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin34 = {34,"gpio34",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin34 = &SysfsGpioPin34;

void gpioPin34_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin34, fd, events);
}

le_result_t le_gpioPin34_SetInput (le_gpioPin34_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin34, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin34_SetPushPullOutput (le_gpioPin34_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin34, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin34_SetOpenDrainOutput (le_gpioPin34_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin34, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin34_SetTriStateOutput (le_gpioPin34_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin34, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin34_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin34, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin34_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin34, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin34_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin34, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin34_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin34);
}

le_result_t le_gpioPin34_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin34);
}

bool le_gpioPin34_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin34);
}

le_result_t le_gpioPin34_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin34);
}

bool le_gpioPin34_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin34);
}

bool le_gpioPin34_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin34);
}

bool le_gpioPin34_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin34);
}

le_gpioPin34_Edge_t le_gpioPin34_GetEdgeSense (void)
{
    return (le_gpioPin34_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin34);
}

le_gpioPin34_Polarity_t le_gpioPin34_GetPolarity (void)
{
    return (le_gpioPin34_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin34);
}

le_gpioPin34_PullUpDown_t le_gpioPin34_GetPullUpDown (void)
{
    return (le_gpioPin34_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin34);
}

le_gpioPin34_ChangeEventHandlerRef_t le_gpioPin34_AddChangeEventHandler
(
    le_gpioPin34_Edge_t trigger,
    le_gpioPin34_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin34_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin34, gpioPin34_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin34_RemoveChangeEventHandler(le_gpioPin34_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin34, addHandlerRef);
}

le_result_t le_gpioPin34_SetEdgeSense (le_gpioPin34_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin34, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin34_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin34);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin34 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin35 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin35 = {35,"gpio35",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin35 = &SysfsGpioPin35;

void gpioPin35_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin35, fd, events);
}

le_result_t le_gpioPin35_SetInput (le_gpioPin35_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin35, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin35_SetPushPullOutput (le_gpioPin35_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin35, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin35_SetOpenDrainOutput (le_gpioPin35_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin35, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin35_SetTriStateOutput (le_gpioPin35_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin35, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin35_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin35, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin35_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin35, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin35_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin35, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin35_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin35);
}

le_result_t le_gpioPin35_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin35);
}

bool le_gpioPin35_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin35);
}

le_result_t le_gpioPin35_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin35);
}

bool le_gpioPin35_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin35);
}

bool le_gpioPin35_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin35);
}

bool le_gpioPin35_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin35);
}

le_gpioPin35_Edge_t le_gpioPin35_GetEdgeSense (void)
{
    return (le_gpioPin35_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin35);
}

le_gpioPin35_Polarity_t le_gpioPin35_GetPolarity (void)
{
    return (le_gpioPin35_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin35);
}

le_gpioPin35_PullUpDown_t le_gpioPin35_GetPullUpDown (void)
{
    return (le_gpioPin35_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin35);
}

le_gpioPin35_ChangeEventHandlerRef_t le_gpioPin35_AddChangeEventHandler
(
    le_gpioPin35_Edge_t trigger,
    le_gpioPin35_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin35_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin35, gpioPin35_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin35_RemoveChangeEventHandler(le_gpioPin35_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin35, addHandlerRef);
}

le_result_t le_gpioPin35_SetEdgeSense (le_gpioPin35_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin35, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin35_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin35);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin35 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin36 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin36 = {36,"gpio36",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin36 = &SysfsGpioPin36;

void gpioPin36_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin36, fd, events);
}

le_result_t le_gpioPin36_SetInput (le_gpioPin36_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin36, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin36_SetPushPullOutput (le_gpioPin36_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin36, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin36_SetOpenDrainOutput (le_gpioPin36_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin36, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin36_SetTriStateOutput (le_gpioPin36_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin36, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin36_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin36, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin36_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin36, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin36_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin36, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin36_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin36);
}

le_result_t le_gpioPin36_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin36);
}

bool le_gpioPin36_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin36);
}

le_result_t le_gpioPin36_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin36);
}

bool le_gpioPin36_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin36);
}

bool le_gpioPin36_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin36);
}

bool le_gpioPin36_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin36);
}

le_gpioPin36_Edge_t le_gpioPin36_GetEdgeSense (void)
{
    return (le_gpioPin36_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin36);
}

le_gpioPin36_Polarity_t le_gpioPin36_GetPolarity (void)
{
    return (le_gpioPin36_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin36);
}

le_gpioPin36_PullUpDown_t le_gpioPin36_GetPullUpDown (void)
{
    return (le_gpioPin36_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin36);
}

le_gpioPin36_ChangeEventHandlerRef_t le_gpioPin36_AddChangeEventHandler
(
    le_gpioPin36_Edge_t trigger,
    le_gpioPin36_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin36_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin36, gpioPin36_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin36_RemoveChangeEventHandler(le_gpioPin36_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin36, addHandlerRef);
}

le_result_t le_gpioPin36_SetEdgeSense (le_gpioPin36_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin36, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin36_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin36);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin36 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin37 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin37 = {37,"gpio37",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin37 = &SysfsGpioPin37;

void gpioPin37_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin37, fd, events);
}

le_result_t le_gpioPin37_SetInput (le_gpioPin37_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin37, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin37_SetPushPullOutput (le_gpioPin37_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin37, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin37_SetOpenDrainOutput (le_gpioPin37_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin37, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin37_SetTriStateOutput (le_gpioPin37_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin37, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin37_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin37, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin37_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin37, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin37_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin37, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin37_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin37);
}

le_result_t le_gpioPin37_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin37);
}

bool le_gpioPin37_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin37);
}

le_result_t le_gpioPin37_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin37);
}

bool le_gpioPin37_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin37);
}

bool le_gpioPin37_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin37);
}

bool le_gpioPin37_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin37);
}

le_gpioPin37_Edge_t le_gpioPin37_GetEdgeSense (void)
{
    return (le_gpioPin37_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin37);
}

le_gpioPin37_Polarity_t le_gpioPin37_GetPolarity (void)
{
    return (le_gpioPin37_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin37);
}

le_gpioPin37_PullUpDown_t le_gpioPin37_GetPullUpDown (void)
{
    return (le_gpioPin37_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin37);
}

le_gpioPin37_ChangeEventHandlerRef_t le_gpioPin37_AddChangeEventHandler
(
    le_gpioPin37_Edge_t trigger,
    le_gpioPin37_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin37_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin37, gpioPin37_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin37_RemoveChangeEventHandler(le_gpioPin37_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin37, addHandlerRef);
}

le_result_t le_gpioPin37_SetEdgeSense (le_gpioPin37_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin37, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin37_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin37);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin37 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin38 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin38 = {38,"gpio38",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin38 = &SysfsGpioPin38;

void gpioPin38_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin38, fd, events);
}

le_result_t le_gpioPin38_SetInput (le_gpioPin38_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin38, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin38_SetPushPullOutput (le_gpioPin38_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin38, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin38_SetOpenDrainOutput (le_gpioPin38_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin38, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin38_SetTriStateOutput (le_gpioPin38_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin38, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin38_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin38, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin38_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin38, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin38_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin38, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin38_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin38);
}

le_result_t le_gpioPin38_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin38);
}

bool le_gpioPin38_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin38);
}

le_result_t le_gpioPin38_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin38);
}

bool le_gpioPin38_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin38);
}

bool le_gpioPin38_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin38);
}

bool le_gpioPin38_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin38);
}

le_gpioPin38_Edge_t le_gpioPin38_GetEdgeSense (void)
{
    return (le_gpioPin38_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin38);
}

le_gpioPin38_Polarity_t le_gpioPin38_GetPolarity (void)
{
    return (le_gpioPin38_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin38);
}

le_gpioPin38_PullUpDown_t le_gpioPin38_GetPullUpDown (void)
{
    return (le_gpioPin38_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin38);
}

le_gpioPin38_ChangeEventHandlerRef_t le_gpioPin38_AddChangeEventHandler
(
    le_gpioPin38_Edge_t trigger,
    le_gpioPin38_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin38_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin38, gpioPin38_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin38_RemoveChangeEventHandler(le_gpioPin38_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin38, addHandlerRef);
}

le_result_t le_gpioPin38_SetEdgeSense (le_gpioPin38_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin38, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin38_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin38);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin38 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin39 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin39 = {39,"gpio39",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin39 = &SysfsGpioPin39;

void gpioPin39_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin39, fd, events);
}

le_result_t le_gpioPin39_SetInput (le_gpioPin39_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin39, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin39_SetPushPullOutput (le_gpioPin39_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin39, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin39_SetOpenDrainOutput (le_gpioPin39_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin39, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin39_SetTriStateOutput (le_gpioPin39_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin39, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin39_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin39, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin39_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin39, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin39_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin39, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin39_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin39);
}

le_result_t le_gpioPin39_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin39);
}

bool le_gpioPin39_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin39);
}

le_result_t le_gpioPin39_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin39);
}

bool le_gpioPin39_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin39);
}

bool le_gpioPin39_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin39);
}

bool le_gpioPin39_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin39);
}

le_gpioPin39_Edge_t le_gpioPin39_GetEdgeSense (void)
{
    return (le_gpioPin39_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin39);
}

le_gpioPin39_Polarity_t le_gpioPin39_GetPolarity (void)
{
    return (le_gpioPin39_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin39);
}

le_gpioPin39_PullUpDown_t le_gpioPin39_GetPullUpDown (void)
{
    return (le_gpioPin39_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin39);
}

le_gpioPin39_ChangeEventHandlerRef_t le_gpioPin39_AddChangeEventHandler
(
    le_gpioPin39_Edge_t trigger,
    le_gpioPin39_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin39_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin39, gpioPin39_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin39_RemoveChangeEventHandler(le_gpioPin39_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin39, addHandlerRef);
}

le_result_t le_gpioPin39_SetEdgeSense (le_gpioPin39_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin39, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin39_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin39);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin39 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin40 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin40 = {40,"gpio40",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin40 = &SysfsGpioPin40;

void gpioPin40_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin40, fd, events);
}

le_result_t le_gpioPin40_SetInput (le_gpioPin40_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin40, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin40_SetPushPullOutput (le_gpioPin40_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin40, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin40_SetOpenDrainOutput (le_gpioPin40_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin40, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin40_SetTriStateOutput (le_gpioPin40_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin40, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin40_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin40, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin40_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin40, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin40_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin40, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin40_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin40);
}

le_result_t le_gpioPin40_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin40);
}

bool le_gpioPin40_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin40);
}

le_result_t le_gpioPin40_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin40);
}

bool le_gpioPin40_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin40);
}

bool le_gpioPin40_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin40);
}

bool le_gpioPin40_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin40);
}

le_gpioPin40_Edge_t le_gpioPin40_GetEdgeSense (void)
{
    return (le_gpioPin40_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin40);
}

le_gpioPin40_Polarity_t le_gpioPin40_GetPolarity (void)
{
    return (le_gpioPin40_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin40);
}

le_gpioPin40_PullUpDown_t le_gpioPin40_GetPullUpDown (void)
{
    return (le_gpioPin40_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin40);
}

le_gpioPin40_ChangeEventHandlerRef_t le_gpioPin40_AddChangeEventHandler
(
    le_gpioPin40_Edge_t trigger,
    le_gpioPin40_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin40_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin40, gpioPin40_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin40_RemoveChangeEventHandler(le_gpioPin40_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin40, addHandlerRef);
}

le_result_t le_gpioPin40_SetEdgeSense (le_gpioPin40_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin40, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin40_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin40);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin40 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin41 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin41 = {41,"gpio41",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin41 = &SysfsGpioPin41;

void gpioPin41_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin41, fd, events);
}

le_result_t le_gpioPin41_SetInput (le_gpioPin41_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin41, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin41_SetPushPullOutput (le_gpioPin41_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin41, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin41_SetOpenDrainOutput (le_gpioPin41_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin41, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin41_SetTriStateOutput (le_gpioPin41_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin41, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin41_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin41, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin41_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin41, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin41_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin41, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin41_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin41);
}

le_result_t le_gpioPin41_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin41);
}

bool le_gpioPin41_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin41);
}

le_result_t le_gpioPin41_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin41);
}

bool le_gpioPin41_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin41);
}

bool le_gpioPin41_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin41);
}

bool le_gpioPin41_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin41);
}

le_gpioPin41_Edge_t le_gpioPin41_GetEdgeSense (void)
{
    return (le_gpioPin41_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin41);
}

le_gpioPin41_Polarity_t le_gpioPin41_GetPolarity (void)
{
    return (le_gpioPin41_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin41);
}

le_gpioPin41_PullUpDown_t le_gpioPin41_GetPullUpDown (void)
{
    return (le_gpioPin41_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin41);
}

le_gpioPin41_ChangeEventHandlerRef_t le_gpioPin41_AddChangeEventHandler
(
    le_gpioPin41_Edge_t trigger,
    le_gpioPin41_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin41_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin41, gpioPin41_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin41_RemoveChangeEventHandler(le_gpioPin41_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin41, addHandlerRef);
}

le_result_t le_gpioPin41_SetEdgeSense (le_gpioPin41_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin41, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin41_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin41);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin41 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin42 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin42 = {42,"gpio42",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin42 = &SysfsGpioPin42;

void gpioPin42_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin42, fd, events);
}

le_result_t le_gpioPin42_SetInput (le_gpioPin42_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin42, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin42_SetPushPullOutput (le_gpioPin42_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin42, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin42_SetOpenDrainOutput (le_gpioPin42_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin42, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin42_SetTriStateOutput (le_gpioPin42_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin42, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin42_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin42, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin42_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin42, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin42_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin42, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin42_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin42);
}

le_result_t le_gpioPin42_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin42);
}

bool le_gpioPin42_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin42);
}

le_result_t le_gpioPin42_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin42);
}

bool le_gpioPin42_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin42);
}

bool le_gpioPin42_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin42);
}

bool le_gpioPin42_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin42);
}

le_gpioPin42_Edge_t le_gpioPin42_GetEdgeSense (void)
{
    return (le_gpioPin42_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin42);
}

le_gpioPin42_Polarity_t le_gpioPin42_GetPolarity (void)
{
    return (le_gpioPin42_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin42);
}

le_gpioPin42_PullUpDown_t le_gpioPin42_GetPullUpDown (void)
{
    return (le_gpioPin42_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin42);
}

le_gpioPin42_ChangeEventHandlerRef_t le_gpioPin42_AddChangeEventHandler
(
    le_gpioPin42_Edge_t trigger,
    le_gpioPin42_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin42_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin42, gpioPin42_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin42_RemoveChangeEventHandler(le_gpioPin42_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin42, addHandlerRef);
}

le_result_t le_gpioPin42_SetEdgeSense (le_gpioPin42_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin42, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin42_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin42);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin42 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin43 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin43 = {43,"gpio43",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin43 = &SysfsGpioPin43;

void gpioPin43_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin43, fd, events);
}

le_result_t le_gpioPin43_SetInput (le_gpioPin43_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin43, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin43_SetPushPullOutput (le_gpioPin43_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin43, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin43_SetOpenDrainOutput (le_gpioPin43_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin43, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin43_SetTriStateOutput (le_gpioPin43_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin43, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin43_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin43, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin43_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin43, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin43_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin43, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin43_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin43);
}

le_result_t le_gpioPin43_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin43);
}

bool le_gpioPin43_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin43);
}

le_result_t le_gpioPin43_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin43);
}

bool le_gpioPin43_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin43);
}

bool le_gpioPin43_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin43);
}

bool le_gpioPin43_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin43);
}

le_gpioPin43_Edge_t le_gpioPin43_GetEdgeSense (void)
{
    return (le_gpioPin43_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin43);
}

le_gpioPin43_Polarity_t le_gpioPin43_GetPolarity (void)
{
    return (le_gpioPin43_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin43);
}

le_gpioPin43_PullUpDown_t le_gpioPin43_GetPullUpDown (void)
{
    return (le_gpioPin43_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin43);
}

le_gpioPin43_ChangeEventHandlerRef_t le_gpioPin43_AddChangeEventHandler
(
    le_gpioPin43_Edge_t trigger,
    le_gpioPin43_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin43_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin43, gpioPin43_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin43_RemoveChangeEventHandler(le_gpioPin43_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin43, addHandlerRef);
}

le_result_t le_gpioPin43_SetEdgeSense (le_gpioPin43_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin43, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin43_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin43);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin43 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin44 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin44 = {44,"gpio44",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin44 = &SysfsGpioPin44;

void gpioPin44_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin44, fd, events);
}

le_result_t le_gpioPin44_SetInput (le_gpioPin44_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin44, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin44_SetPushPullOutput (le_gpioPin44_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin44, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin44_SetOpenDrainOutput (le_gpioPin44_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin44, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin44_SetTriStateOutput (le_gpioPin44_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin44, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin44_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin44, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin44_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin44, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin44_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin44, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin44_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin44);
}

le_result_t le_gpioPin44_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin44);
}

bool le_gpioPin44_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin44);
}

le_result_t le_gpioPin44_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin44);
}

bool le_gpioPin44_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin44);
}

bool le_gpioPin44_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin44);
}

bool le_gpioPin44_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin44);
}

le_gpioPin44_Edge_t le_gpioPin44_GetEdgeSense (void)
{
    return (le_gpioPin44_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin44);
}

le_gpioPin44_Polarity_t le_gpioPin44_GetPolarity (void)
{
    return (le_gpioPin44_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin44);
}

le_gpioPin44_PullUpDown_t le_gpioPin44_GetPullUpDown (void)
{
    return (le_gpioPin44_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin44);
}

le_gpioPin44_ChangeEventHandlerRef_t le_gpioPin44_AddChangeEventHandler
(
    le_gpioPin44_Edge_t trigger,
    le_gpioPin44_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin44_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin44, gpioPin44_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin44_RemoveChangeEventHandler(le_gpioPin44_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin44, addHandlerRef);
}

le_result_t le_gpioPin44_SetEdgeSense (le_gpioPin44_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin44, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin44_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin44);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin44 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin45 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin45 = {45,"gpio45",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin45 = &SysfsGpioPin45;

void gpioPin45_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin45, fd, events);
}

le_result_t le_gpioPin45_SetInput (le_gpioPin45_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin45, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin45_SetPushPullOutput (le_gpioPin45_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin45, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin45_SetOpenDrainOutput (le_gpioPin45_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin45, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin45_SetTriStateOutput (le_gpioPin45_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin45, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin45_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin45, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin45_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin45, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin45_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin45, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin45_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin45);
}

le_result_t le_gpioPin45_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin45);
}

bool le_gpioPin45_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin45);
}

le_result_t le_gpioPin45_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin45);
}

bool le_gpioPin45_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin45);
}

bool le_gpioPin45_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin45);
}

bool le_gpioPin45_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin45);
}

le_gpioPin45_Edge_t le_gpioPin45_GetEdgeSense (void)
{
    return (le_gpioPin45_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin45);
}

le_gpioPin45_Polarity_t le_gpioPin45_GetPolarity (void)
{
    return (le_gpioPin45_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin45);
}

le_gpioPin45_PullUpDown_t le_gpioPin45_GetPullUpDown (void)
{
    return (le_gpioPin45_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin45);
}

le_gpioPin45_ChangeEventHandlerRef_t le_gpioPin45_AddChangeEventHandler
(
    le_gpioPin45_Edge_t trigger,
    le_gpioPin45_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin45_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin45, gpioPin45_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin45_RemoveChangeEventHandler(le_gpioPin45_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin45, addHandlerRef);
}

le_result_t le_gpioPin45_SetEdgeSense (le_gpioPin45_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin45, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin45_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin45);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin45 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin46 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin46 = {46,"gpio46",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin46 = &SysfsGpioPin46;

void gpioPin46_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin46, fd, events);
}

le_result_t le_gpioPin46_SetInput (le_gpioPin46_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin46, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin46_SetPushPullOutput (le_gpioPin46_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin46, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin46_SetOpenDrainOutput (le_gpioPin46_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin46, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin46_SetTriStateOutput (le_gpioPin46_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin46, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin46_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin46, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin46_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin46, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin46_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin46, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin46_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin46);
}

le_result_t le_gpioPin46_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin46);
}

bool le_gpioPin46_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin46);
}

le_result_t le_gpioPin46_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin46);
}

bool le_gpioPin46_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin46);
}

bool le_gpioPin46_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin46);
}

bool le_gpioPin46_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin46);
}

le_gpioPin46_Edge_t le_gpioPin46_GetEdgeSense (void)
{
    return (le_gpioPin46_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin46);
}

le_gpioPin46_Polarity_t le_gpioPin46_GetPolarity (void)
{
    return (le_gpioPin46_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin46);
}

le_gpioPin46_PullUpDown_t le_gpioPin46_GetPullUpDown (void)
{
    return (le_gpioPin46_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin46);
}

le_gpioPin46_ChangeEventHandlerRef_t le_gpioPin46_AddChangeEventHandler
(
    le_gpioPin46_Edge_t trigger,
    le_gpioPin46_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin46_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin46, gpioPin46_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin46_RemoveChangeEventHandler(le_gpioPin46_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin46, addHandlerRef);
}

le_result_t le_gpioPin46_SetEdgeSense (le_gpioPin46_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin46, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin46_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin46);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin46 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin47 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin47 = {47,"gpio47",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin47 = &SysfsGpioPin47;

void gpioPin47_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin47, fd, events);
}

le_result_t le_gpioPin47_SetInput (le_gpioPin47_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin47, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin47_SetPushPullOutput (le_gpioPin47_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin47, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin47_SetOpenDrainOutput (le_gpioPin47_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin47, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin47_SetTriStateOutput (le_gpioPin47_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin47, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin47_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin47, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin47_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin47, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin47_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin47, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin47_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin47);
}

le_result_t le_gpioPin47_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin47);
}

bool le_gpioPin47_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin47);
}

le_result_t le_gpioPin47_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin47);
}

bool le_gpioPin47_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin47);
}

bool le_gpioPin47_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin47);
}

bool le_gpioPin47_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin47);
}

le_gpioPin47_Edge_t le_gpioPin47_GetEdgeSense (void)
{
    return (le_gpioPin47_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin47);
}

le_gpioPin47_Polarity_t le_gpioPin47_GetPolarity (void)
{
    return (le_gpioPin47_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin47);
}

le_gpioPin47_PullUpDown_t le_gpioPin47_GetPullUpDown (void)
{
    return (le_gpioPin47_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin47);
}

le_gpioPin47_ChangeEventHandlerRef_t le_gpioPin47_AddChangeEventHandler
(
    le_gpioPin47_Edge_t trigger,
    le_gpioPin47_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin47_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin47, gpioPin47_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin47_RemoveChangeEventHandler(le_gpioPin47_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin47, addHandlerRef);
}

le_result_t le_gpioPin47_SetEdgeSense (le_gpioPin47_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin47, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin47_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin47);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin47 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin48 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin48 = {48,"gpio48",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin48 = &SysfsGpioPin48;

void gpioPin48_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin48, fd, events);
}

le_result_t le_gpioPin48_SetInput (le_gpioPin48_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin48, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin48_SetPushPullOutput (le_gpioPin48_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin48, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin48_SetOpenDrainOutput (le_gpioPin48_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin48, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin48_SetTriStateOutput (le_gpioPin48_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin48, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin48_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin48, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin48_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin48, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin48_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin48, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin48_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin48);
}

le_result_t le_gpioPin48_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin48);
}

bool le_gpioPin48_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin48);
}

le_result_t le_gpioPin48_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin48);
}

bool le_gpioPin48_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin48);
}

bool le_gpioPin48_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin48);
}

bool le_gpioPin48_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin48);
}

le_gpioPin48_Edge_t le_gpioPin48_GetEdgeSense (void)
{
    return (le_gpioPin48_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin48);
}

le_gpioPin48_Polarity_t le_gpioPin48_GetPolarity (void)
{
    return (le_gpioPin48_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin48);
}

le_gpioPin48_PullUpDown_t le_gpioPin48_GetPullUpDown (void)
{
    return (le_gpioPin48_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin48);
}

le_gpioPin48_ChangeEventHandlerRef_t le_gpioPin48_AddChangeEventHandler
(
    le_gpioPin48_Edge_t trigger,
    le_gpioPin48_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin48_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin48, gpioPin48_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin48_RemoveChangeEventHandler(le_gpioPin48_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin48, addHandlerRef);
}

le_result_t le_gpioPin48_SetEdgeSense (le_gpioPin48_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin48, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin48_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin48);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin48 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin49 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin49 = {49,"gpio49",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin49 = &SysfsGpioPin49;

void gpioPin49_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin49, fd, events);
}

le_result_t le_gpioPin49_SetInput (le_gpioPin49_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin49, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin49_SetPushPullOutput (le_gpioPin49_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin49, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin49_SetOpenDrainOutput (le_gpioPin49_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin49, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin49_SetTriStateOutput (le_gpioPin49_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin49, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin49_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin49, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin49_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin49, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin49_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin49, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin49_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin49);
}

le_result_t le_gpioPin49_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin49);
}

bool le_gpioPin49_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin49);
}

le_result_t le_gpioPin49_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin49);
}

bool le_gpioPin49_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin49);
}

bool le_gpioPin49_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin49);
}

bool le_gpioPin49_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin49);
}

le_gpioPin49_Edge_t le_gpioPin49_GetEdgeSense (void)
{
    return (le_gpioPin49_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin49);
}

le_gpioPin49_Polarity_t le_gpioPin49_GetPolarity (void)
{
    return (le_gpioPin49_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin49);
}

le_gpioPin49_PullUpDown_t le_gpioPin49_GetPullUpDown (void)
{
    return (le_gpioPin49_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin49);
}

le_gpioPin49_ChangeEventHandlerRef_t le_gpioPin49_AddChangeEventHandler
(
    le_gpioPin49_Edge_t trigger,
    le_gpioPin49_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin49_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin49, gpioPin49_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin49_RemoveChangeEventHandler(le_gpioPin49_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin49, addHandlerRef);
}

le_result_t le_gpioPin49_SetEdgeSense (le_gpioPin49_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin49, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin49_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin49);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin49 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin50 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin50 = {50,"gpio50",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin50 = &SysfsGpioPin50;

void gpioPin50_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin50, fd, events);
}

le_result_t le_gpioPin50_SetInput (le_gpioPin50_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin50, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin50_SetPushPullOutput (le_gpioPin50_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin50, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin50_SetOpenDrainOutput (le_gpioPin50_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin50, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin50_SetTriStateOutput (le_gpioPin50_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin50, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin50_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin50, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin50_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin50, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin50_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin50, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin50_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin50);
}

le_result_t le_gpioPin50_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin50);
}

bool le_gpioPin50_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin50);
}

le_result_t le_gpioPin50_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin50);
}

bool le_gpioPin50_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin50);
}

bool le_gpioPin50_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin50);
}

bool le_gpioPin50_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin50);
}

le_gpioPin50_Edge_t le_gpioPin50_GetEdgeSense (void)
{
    return (le_gpioPin50_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin50);
}

le_gpioPin50_Polarity_t le_gpioPin50_GetPolarity (void)
{
    return (le_gpioPin50_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin50);
}

le_gpioPin50_PullUpDown_t le_gpioPin50_GetPullUpDown (void)
{
    return (le_gpioPin50_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin50);
}

le_gpioPin50_ChangeEventHandlerRef_t le_gpioPin50_AddChangeEventHandler
(
    le_gpioPin50_Edge_t trigger,
    le_gpioPin50_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin50_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin50, gpioPin50_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin50_RemoveChangeEventHandler(le_gpioPin50_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin50, addHandlerRef);
}

le_result_t le_gpioPin50_SetEdgeSense (le_gpioPin50_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin50, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin50_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin50);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin50 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin51 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin51 = {51,"gpio51",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin51 = &SysfsGpioPin51;

void gpioPin51_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin51, fd, events);
}

le_result_t le_gpioPin51_SetInput (le_gpioPin51_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin51, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin51_SetPushPullOutput (le_gpioPin51_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin51, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin51_SetOpenDrainOutput (le_gpioPin51_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin51, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin51_SetTriStateOutput (le_gpioPin51_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin51, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin51_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin51, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin51_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin51, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin51_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin51, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin51_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin51);
}

le_result_t le_gpioPin51_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin51);
}

bool le_gpioPin51_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin51);
}

le_result_t le_gpioPin51_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin51);
}

bool le_gpioPin51_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin51);
}

bool le_gpioPin51_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin51);
}

bool le_gpioPin51_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin51);
}

le_gpioPin51_Edge_t le_gpioPin51_GetEdgeSense (void)
{
    return (le_gpioPin51_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin51);
}

le_gpioPin51_Polarity_t le_gpioPin51_GetPolarity (void)
{
    return (le_gpioPin51_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin51);
}

le_gpioPin51_PullUpDown_t le_gpioPin51_GetPullUpDown (void)
{
    return (le_gpioPin51_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin51);
}

le_gpioPin51_ChangeEventHandlerRef_t le_gpioPin51_AddChangeEventHandler
(
    le_gpioPin51_Edge_t trigger,
    le_gpioPin51_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin51_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin51, gpioPin51_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin51_RemoveChangeEventHandler(le_gpioPin51_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin51, addHandlerRef);
}

le_result_t le_gpioPin51_SetEdgeSense (le_gpioPin51_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin51, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin51_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin51);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin51 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin52 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin52 = {52,"gpio52",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin52 = &SysfsGpioPin52;

void gpioPin52_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin52, fd, events);
}

le_result_t le_gpioPin52_SetInput (le_gpioPin52_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin52, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin52_SetPushPullOutput (le_gpioPin52_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin52, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin52_SetOpenDrainOutput (le_gpioPin52_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin52, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin52_SetTriStateOutput (le_gpioPin52_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin52, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin52_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin52, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin52_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin52, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin52_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin52, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin52_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin52);
}

le_result_t le_gpioPin52_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin52);
}

bool le_gpioPin52_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin52);
}

le_result_t le_gpioPin52_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin52);
}

bool le_gpioPin52_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin52);
}

bool le_gpioPin52_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin52);
}

bool le_gpioPin52_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin52);
}

le_gpioPin52_Edge_t le_gpioPin52_GetEdgeSense (void)
{
    return (le_gpioPin52_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin52);
}

le_gpioPin52_Polarity_t le_gpioPin52_GetPolarity (void)
{
    return (le_gpioPin52_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin52);
}

le_gpioPin52_PullUpDown_t le_gpioPin52_GetPullUpDown (void)
{
    return (le_gpioPin52_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin52);
}

le_gpioPin52_ChangeEventHandlerRef_t le_gpioPin52_AddChangeEventHandler
(
    le_gpioPin52_Edge_t trigger,
    le_gpioPin52_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin52_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin52, gpioPin52_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin52_RemoveChangeEventHandler(le_gpioPin52_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin52, addHandlerRef);
}

le_result_t le_gpioPin52_SetEdgeSense (le_gpioPin52_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin52, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin52_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin52);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin52 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin53 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin53 = {53,"gpio53",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin53 = &SysfsGpioPin53;

void gpioPin53_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin53, fd, events);
}

le_result_t le_gpioPin53_SetInput (le_gpioPin53_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin53, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin53_SetPushPullOutput (le_gpioPin53_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin53, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin53_SetOpenDrainOutput (le_gpioPin53_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin53, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin53_SetTriStateOutput (le_gpioPin53_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin53, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin53_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin53, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin53_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin53, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin53_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin53, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin53_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin53);
}

le_result_t le_gpioPin53_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin53);
}

bool le_gpioPin53_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin53);
}

le_result_t le_gpioPin53_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin53);
}

bool le_gpioPin53_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin53);
}

bool le_gpioPin53_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin53);
}

bool le_gpioPin53_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin53);
}

le_gpioPin53_Edge_t le_gpioPin53_GetEdgeSense (void)
{
    return (le_gpioPin53_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin53);
}

le_gpioPin53_Polarity_t le_gpioPin53_GetPolarity (void)
{
    return (le_gpioPin53_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin53);
}

le_gpioPin53_PullUpDown_t le_gpioPin53_GetPullUpDown (void)
{
    return (le_gpioPin53_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin53);
}

le_gpioPin53_ChangeEventHandlerRef_t le_gpioPin53_AddChangeEventHandler
(
    le_gpioPin53_Edge_t trigger,
    le_gpioPin53_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin53_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin53, gpioPin53_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin53_RemoveChangeEventHandler(le_gpioPin53_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin53, addHandlerRef);
}

le_result_t le_gpioPin53_SetEdgeSense (le_gpioPin53_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin53, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin53_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin53);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin53 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin54 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin54 = {54,"gpio54",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin54 = &SysfsGpioPin54;

void gpioPin54_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin54, fd, events);
}

le_result_t le_gpioPin54_SetInput (le_gpioPin54_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin54, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin54_SetPushPullOutput (le_gpioPin54_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin54, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin54_SetOpenDrainOutput (le_gpioPin54_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin54, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin54_SetTriStateOutput (le_gpioPin54_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin54, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin54_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin54, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin54_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin54, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin54_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin54, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin54_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin54);
}

le_result_t le_gpioPin54_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin54);
}

bool le_gpioPin54_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin54);
}

le_result_t le_gpioPin54_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin54);
}

bool le_gpioPin54_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin54);
}

bool le_gpioPin54_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin54);
}

bool le_gpioPin54_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin54);
}

le_gpioPin54_Edge_t le_gpioPin54_GetEdgeSense (void)
{
    return (le_gpioPin54_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin54);
}

le_gpioPin54_Polarity_t le_gpioPin54_GetPolarity (void)
{
    return (le_gpioPin54_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin54);
}

le_gpioPin54_PullUpDown_t le_gpioPin54_GetPullUpDown (void)
{
    return (le_gpioPin54_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin54);
}

le_gpioPin54_ChangeEventHandlerRef_t le_gpioPin54_AddChangeEventHandler
(
    le_gpioPin54_Edge_t trigger,
    le_gpioPin54_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin54_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin54, gpioPin54_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin54_RemoveChangeEventHandler(le_gpioPin54_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin54, addHandlerRef);
}

le_result_t le_gpioPin54_SetEdgeSense (le_gpioPin54_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin54, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin54_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin54);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin54 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin55 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin55 = {55,"gpio55",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin55 = &SysfsGpioPin55;

void gpioPin55_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin55, fd, events);
}

le_result_t le_gpioPin55_SetInput (le_gpioPin55_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin55, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin55_SetPushPullOutput (le_gpioPin55_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin55, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin55_SetOpenDrainOutput (le_gpioPin55_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin55, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin55_SetTriStateOutput (le_gpioPin55_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin55, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin55_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin55, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin55_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin55, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin55_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin55, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin55_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin55);
}

le_result_t le_gpioPin55_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin55);
}

bool le_gpioPin55_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin55);
}

le_result_t le_gpioPin55_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin55);
}

bool le_gpioPin55_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin55);
}

bool le_gpioPin55_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin55);
}

bool le_gpioPin55_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin55);
}

le_gpioPin55_Edge_t le_gpioPin55_GetEdgeSense (void)
{
    return (le_gpioPin55_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin55);
}

le_gpioPin55_Polarity_t le_gpioPin55_GetPolarity (void)
{
    return (le_gpioPin55_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin55);
}

le_gpioPin55_PullUpDown_t le_gpioPin55_GetPullUpDown (void)
{
    return (le_gpioPin55_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin55);
}

le_gpioPin55_ChangeEventHandlerRef_t le_gpioPin55_AddChangeEventHandler
(
    le_gpioPin55_Edge_t trigger,
    le_gpioPin55_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin55_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin55, gpioPin55_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin55_RemoveChangeEventHandler(le_gpioPin55_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin55, addHandlerRef);
}

le_result_t le_gpioPin55_SetEdgeSense (le_gpioPin55_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin55, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin55_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin55);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin55 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin56 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin56 = {56,"gpio56",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin56 = &SysfsGpioPin56;

void gpioPin56_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin56, fd, events);
}

le_result_t le_gpioPin56_SetInput (le_gpioPin56_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin56, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin56_SetPushPullOutput (le_gpioPin56_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin56, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin56_SetOpenDrainOutput (le_gpioPin56_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin56, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin56_SetTriStateOutput (le_gpioPin56_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin56, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin56_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin56, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin56_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin56, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin56_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin56, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin56_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin56);
}

le_result_t le_gpioPin56_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin56);
}

bool le_gpioPin56_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin56);
}

le_result_t le_gpioPin56_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin56);
}

bool le_gpioPin56_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin56);
}

bool le_gpioPin56_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin56);
}

bool le_gpioPin56_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin56);
}

le_gpioPin56_Edge_t le_gpioPin56_GetEdgeSense (void)
{
    return (le_gpioPin56_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin56);
}

le_gpioPin56_Polarity_t le_gpioPin56_GetPolarity (void)
{
    return (le_gpioPin56_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin56);
}

le_gpioPin56_PullUpDown_t le_gpioPin56_GetPullUpDown (void)
{
    return (le_gpioPin56_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin56);
}

le_gpioPin56_ChangeEventHandlerRef_t le_gpioPin56_AddChangeEventHandler
(
    le_gpioPin56_Edge_t trigger,
    le_gpioPin56_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin56_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin56, gpioPin56_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin56_RemoveChangeEventHandler(le_gpioPin56_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin56, addHandlerRef);
}

le_result_t le_gpioPin56_SetEdgeSense (le_gpioPin56_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin56, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin56_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin56);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin56 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin57 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin57 = {57,"gpio57",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin57 = &SysfsGpioPin57;

void gpioPin57_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin57, fd, events);
}

le_result_t le_gpioPin57_SetInput (le_gpioPin57_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin57, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin57_SetPushPullOutput (le_gpioPin57_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin57, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin57_SetOpenDrainOutput (le_gpioPin57_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin57, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin57_SetTriStateOutput (le_gpioPin57_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin57, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin57_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin57, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin57_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin57, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin57_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin57, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin57_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin57);
}

le_result_t le_gpioPin57_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin57);
}

bool le_gpioPin57_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin57);
}

le_result_t le_gpioPin57_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin57);
}

bool le_gpioPin57_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin57);
}

bool le_gpioPin57_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin57);
}

bool le_gpioPin57_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin57);
}

le_gpioPin57_Edge_t le_gpioPin57_GetEdgeSense (void)
{
    return (le_gpioPin57_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin57);
}

le_gpioPin57_Polarity_t le_gpioPin57_GetPolarity (void)
{
    return (le_gpioPin57_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin57);
}

le_gpioPin57_PullUpDown_t le_gpioPin57_GetPullUpDown (void)
{
    return (le_gpioPin57_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin57);
}

le_gpioPin57_ChangeEventHandlerRef_t le_gpioPin57_AddChangeEventHandler
(
    le_gpioPin57_Edge_t trigger,
    le_gpioPin57_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin57_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin57, gpioPin57_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin57_RemoveChangeEventHandler(le_gpioPin57_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin57, addHandlerRef);
}

le_result_t le_gpioPin57_SetEdgeSense (le_gpioPin57_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin57, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin57_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin57);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin57 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin58 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin58 = {58,"gpio58",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin58 = &SysfsGpioPin58;

void gpioPin58_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin58, fd, events);
}

le_result_t le_gpioPin58_SetInput (le_gpioPin58_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin58, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin58_SetPushPullOutput (le_gpioPin58_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin58, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin58_SetOpenDrainOutput (le_gpioPin58_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin58, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin58_SetTriStateOutput (le_gpioPin58_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin58, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin58_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin58, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin58_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin58, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin58_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin58, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin58_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin58);
}

le_result_t le_gpioPin58_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin58);
}

bool le_gpioPin58_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin58);
}

le_result_t le_gpioPin58_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin58);
}

bool le_gpioPin58_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin58);
}

bool le_gpioPin58_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin58);
}

bool le_gpioPin58_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin58);
}

le_gpioPin58_Edge_t le_gpioPin58_GetEdgeSense (void)
{
    return (le_gpioPin58_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin58);
}

le_gpioPin58_Polarity_t le_gpioPin58_GetPolarity (void)
{
    return (le_gpioPin58_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin58);
}

le_gpioPin58_PullUpDown_t le_gpioPin58_GetPullUpDown (void)
{
    return (le_gpioPin58_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin58);
}

le_gpioPin58_ChangeEventHandlerRef_t le_gpioPin58_AddChangeEventHandler
(
    le_gpioPin58_Edge_t trigger,
    le_gpioPin58_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin58_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin58, gpioPin58_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin58_RemoveChangeEventHandler(le_gpioPin58_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin58, addHandlerRef);
}

le_result_t le_gpioPin58_SetEdgeSense (le_gpioPin58_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin58, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin58_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin58);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin58 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin59 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin59 = {59,"gpio59",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin59 = &SysfsGpioPin59;

void gpioPin59_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin59, fd, events);
}

le_result_t le_gpioPin59_SetInput (le_gpioPin59_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin59, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin59_SetPushPullOutput (le_gpioPin59_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin59, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin59_SetOpenDrainOutput (le_gpioPin59_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin59, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin59_SetTriStateOutput (le_gpioPin59_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin59, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin59_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin59, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin59_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin59, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin59_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin59, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin59_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin59);
}

le_result_t le_gpioPin59_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin59);
}

bool le_gpioPin59_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin59);
}

le_result_t le_gpioPin59_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin59);
}

bool le_gpioPin59_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin59);
}

bool le_gpioPin59_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin59);
}

bool le_gpioPin59_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin59);
}

le_gpioPin59_Edge_t le_gpioPin59_GetEdgeSense (void)
{
    return (le_gpioPin59_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin59);
}

le_gpioPin59_Polarity_t le_gpioPin59_GetPolarity (void)
{
    return (le_gpioPin59_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin59);
}

le_gpioPin59_PullUpDown_t le_gpioPin59_GetPullUpDown (void)
{
    return (le_gpioPin59_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin59);
}

le_gpioPin59_ChangeEventHandlerRef_t le_gpioPin59_AddChangeEventHandler
(
    le_gpioPin59_Edge_t trigger,
    le_gpioPin59_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin59_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin59, gpioPin59_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin59_RemoveChangeEventHandler(le_gpioPin59_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin59, addHandlerRef);
}

le_result_t le_gpioPin59_SetEdgeSense (le_gpioPin59_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin59, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin59_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin59);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin59 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin60 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin60 = {60,"gpio60",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin60 = &SysfsGpioPin60;

void gpioPin60_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin60, fd, events);
}

le_result_t le_gpioPin60_SetInput (le_gpioPin60_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin60, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin60_SetPushPullOutput (le_gpioPin60_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin60, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin60_SetOpenDrainOutput (le_gpioPin60_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin60, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin60_SetTriStateOutput (le_gpioPin60_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin60, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin60_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin60, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin60_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin60, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin60_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin60, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin60_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin60);
}

le_result_t le_gpioPin60_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin60);
}

bool le_gpioPin60_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin60);
}

le_result_t le_gpioPin60_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin60);
}

bool le_gpioPin60_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin60);
}

bool le_gpioPin60_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin60);
}

bool le_gpioPin60_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin60);
}

le_gpioPin60_Edge_t le_gpioPin60_GetEdgeSense (void)
{
    return (le_gpioPin60_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin60);
}

le_gpioPin60_Polarity_t le_gpioPin60_GetPolarity (void)
{
    return (le_gpioPin60_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin60);
}

le_gpioPin60_PullUpDown_t le_gpioPin60_GetPullUpDown (void)
{
    return (le_gpioPin60_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin60);
}

le_gpioPin60_ChangeEventHandlerRef_t le_gpioPin60_AddChangeEventHandler
(
    le_gpioPin60_Edge_t trigger,
    le_gpioPin60_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin60_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin60, gpioPin60_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin60_RemoveChangeEventHandler(le_gpioPin60_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin60, addHandlerRef);
}

le_result_t le_gpioPin60_SetEdgeSense (le_gpioPin60_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin60, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin60_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin60);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin60 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin61 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin61 = {61,"gpio61",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin61 = &SysfsGpioPin61;

void gpioPin61_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin61, fd, events);
}

le_result_t le_gpioPin61_SetInput (le_gpioPin61_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin61, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin61_SetPushPullOutput (le_gpioPin61_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin61, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin61_SetOpenDrainOutput (le_gpioPin61_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin61, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin61_SetTriStateOutput (le_gpioPin61_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin61, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin61_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin61, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin61_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin61, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin61_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin61, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin61_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin61);
}

le_result_t le_gpioPin61_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin61);
}

bool le_gpioPin61_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin61);
}

le_result_t le_gpioPin61_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin61);
}

bool le_gpioPin61_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin61);
}

bool le_gpioPin61_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin61);
}

bool le_gpioPin61_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin61);
}

le_gpioPin61_Edge_t le_gpioPin61_GetEdgeSense (void)
{
    return (le_gpioPin61_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin61);
}

le_gpioPin61_Polarity_t le_gpioPin61_GetPolarity (void)
{
    return (le_gpioPin61_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin61);
}

le_gpioPin61_PullUpDown_t le_gpioPin61_GetPullUpDown (void)
{
    return (le_gpioPin61_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin61);
}

le_gpioPin61_ChangeEventHandlerRef_t le_gpioPin61_AddChangeEventHandler
(
    le_gpioPin61_Edge_t trigger,
    le_gpioPin61_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin61_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin61, gpioPin61_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin61_RemoveChangeEventHandler(le_gpioPin61_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin61, addHandlerRef);
}

le_result_t le_gpioPin61_SetEdgeSense (le_gpioPin61_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin61, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin61_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin61);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin61 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin62 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin62 = {62,"gpio62",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin62 = &SysfsGpioPin62;

void gpioPin62_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin62, fd, events);
}

le_result_t le_gpioPin62_SetInput (le_gpioPin62_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin62, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin62_SetPushPullOutput (le_gpioPin62_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin62, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin62_SetOpenDrainOutput (le_gpioPin62_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin62, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin62_SetTriStateOutput (le_gpioPin62_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin62, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin62_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin62, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin62_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin62, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin62_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin62, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin62_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin62);
}

le_result_t le_gpioPin62_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin62);
}

bool le_gpioPin62_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin62);
}

le_result_t le_gpioPin62_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin62);
}

bool le_gpioPin62_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin62);
}

bool le_gpioPin62_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin62);
}

bool le_gpioPin62_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin62);
}

le_gpioPin62_Edge_t le_gpioPin62_GetEdgeSense (void)
{
    return (le_gpioPin62_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin62);
}

le_gpioPin62_Polarity_t le_gpioPin62_GetPolarity (void)
{
    return (le_gpioPin62_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin62);
}

le_gpioPin62_PullUpDown_t le_gpioPin62_GetPullUpDown (void)
{
    return (le_gpioPin62_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin62);
}

le_gpioPin62_ChangeEventHandlerRef_t le_gpioPin62_AddChangeEventHandler
(
    le_gpioPin62_Edge_t trigger,
    le_gpioPin62_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin62_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin62, gpioPin62_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin62_RemoveChangeEventHandler(le_gpioPin62_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin62, addHandlerRef);
}

le_result_t le_gpioPin62_SetEdgeSense (le_gpioPin62_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin62, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin62_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin62);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin62 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin63 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin63 = {63,"gpio63",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin63 = &SysfsGpioPin63;

void gpioPin63_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin63, fd, events);
}

le_result_t le_gpioPin63_SetInput (le_gpioPin63_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin63, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin63_SetPushPullOutput (le_gpioPin63_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin63, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin63_SetOpenDrainOutput (le_gpioPin63_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin63, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin63_SetTriStateOutput (le_gpioPin63_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin63, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin63_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin63, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin63_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin63, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin63_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin63, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin63_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin63);
}

le_result_t le_gpioPin63_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin63);
}

bool le_gpioPin63_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin63);
}

le_result_t le_gpioPin63_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin63);
}

bool le_gpioPin63_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin63);
}

bool le_gpioPin63_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin63);
}

bool le_gpioPin63_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin63);
}

le_gpioPin63_Edge_t le_gpioPin63_GetEdgeSense (void)
{
    return (le_gpioPin63_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin63);
}

le_gpioPin63_Polarity_t le_gpioPin63_GetPolarity (void)
{
    return (le_gpioPin63_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin63);
}

le_gpioPin63_PullUpDown_t le_gpioPin63_GetPullUpDown (void)
{
    return (le_gpioPin63_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin63);
}

le_gpioPin63_ChangeEventHandlerRef_t le_gpioPin63_AddChangeEventHandler
(
    le_gpioPin63_Edge_t trigger,
    le_gpioPin63_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin63_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin63, gpioPin63_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin63_RemoveChangeEventHandler(le_gpioPin63_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin63, addHandlerRef);
}

le_result_t le_gpioPin63_SetEdgeSense (le_gpioPin63_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin63, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin63_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin63);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin63 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Pin64 boilerplate functions. These pass through all the calls to
 * the generic GPIO functions. This code is generated.
 */
//--------------------------------------------------------------------------------------------------

static struct gpioSysfs_Gpio SysfsGpioPin64 = {64,"gpio64",false,NULL,NULL,NULL,NULL};
static gpioSysfs_GpioRef_t gpioRefPin64 = &SysfsGpioPin64;

void gpioPin64_InputMonitorHandlerFunc (int fd, short events)
{
    gpioSysfs_InputMonitorHandlerFunc(gpioRefPin64, fd, events);
}

le_result_t le_gpioPin64_SetInput (le_gpioPin64_Polarity_t polarity)
{
    return gpioSysfs_SetInput(gpioRefPin64, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin64_SetPushPullOutput (le_gpioPin64_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetPushPullOutput(gpioRefPin64, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin64_SetOpenDrainOutput (le_gpioPin64_Polarity_t polarity, bool value)
{
    return gpioSysfs_SetOpenDrain(gpioRefPin64, (gpioSysfs_ActiveType_t)polarity, value);
}

le_result_t le_gpioPin64_SetTriStateOutput (le_gpioPin64_Polarity_t polarity)
{
    return gpioSysfs_SetTriState(gpioRefPin64, (gpioSysfs_ActiveType_t)polarity);
}

le_result_t le_gpioPin64_EnablePullUp (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin64, SYSFS_PULLUPDOWN_TYPE_UP);
}

le_result_t le_gpioPin64_EnablePullDown (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin64, SYSFS_PULLUPDOWN_TYPE_DOWN);
}

le_result_t le_gpioPin64_DisableResistors (void)
{
    return gpioSysfs_SetPullUpDown(gpioRefPin64, SYSFS_PULLUPDOWN_TYPE_OFF);
}

le_result_t le_gpioPin64_Activate (void)
{
    return gpioSysfs_Activate(gpioRefPin64);
}

le_result_t le_gpioPin64_Deactivate (void)
{
    return gpioSysfs_Deactivate(gpioRefPin64);
}

bool le_gpioPin64_Read (void)
{
    return gpioSysfs_ReadValue(gpioRefPin64);
}

le_result_t le_gpioPin64_SetHighZ (void)
{
    return gpioSysfs_SetHighZ(gpioRefPin64);
}

bool le_gpioPin64_IsActive (void)
{
    return gpioSysfs_IsActive(gpioRefPin64);
}

bool le_gpioPin64_IsInput (void)
{
    return gpioSysfs_IsInput(gpioRefPin64);
}

bool le_gpioPin64_IsOutput (void)
{
    return gpioSysfs_IsOutput(gpioRefPin64);
}

le_gpioPin64_Edge_t le_gpioPin64_GetEdgeSense (void)
{
    return (le_gpioPin64_Edge_t)gpioSysfs_GetEdgeSense(gpioRefPin64);
}

le_gpioPin64_Polarity_t le_gpioPin64_GetPolarity (void)
{
    return (le_gpioPin64_Polarity_t)gpioSysfs_GetPolarity(gpioRefPin64);
}

le_gpioPin64_PullUpDown_t le_gpioPin64_GetPullUpDown (void)
{
    return (le_gpioPin64_PullUpDown_t)gpioSysfs_GetPullUpDown(gpioRefPin64);
}

le_gpioPin64_ChangeEventHandlerRef_t le_gpioPin64_AddChangeEventHandler
(
    le_gpioPin64_Edge_t trigger,
    le_gpioPin64_ChangeCallbackFunc_t handlerPtr,
    void* contextPtr,
    int32_t sampleMs
)
{
    return (le_gpioPin64_ChangeEventHandlerRef_t)gpioSysfs_SetChangeCallback(
                gpioRefPin64, gpioPin64_InputMonitorHandlerFunc,
                (gpioSysfs_EdgeSensivityMode_t)trigger, handlerPtr, contextPtr, sampleMs);
}

void le_gpioPin64_RemoveChangeEventHandler(le_gpioPin64_ChangeEventHandlerRef_t addHandlerRef)
{
    gpioSysfs_RemoveChangeCallback(gpioRefPin64, addHandlerRef);
}

le_result_t le_gpioPin64_SetEdgeSense (le_gpioPin64_Edge_t trigger)
{
    return gpioSysfs_SetEdgeSense(gpioRefPin64, (gpioSysfs_EdgeSensivityMode_t)trigger);
}

le_result_t le_gpioPin64_DisableEdgeSense (void)
{
    return gpioSysfs_DisableEdgeSense(gpioRefPin64);
}

//--------------------------------------------------------------------------------------------------
/**
 * End of Pin64 boilerplate functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * "Global" functions - that apply to whole GPIO functionality, not just
 * one particular GPIO
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Returns total number of GPIO pins in the system
 *
 * @return
 *       number of GPIO pins in the system
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_gpioCfg_GetTotalPinNumber(void)
{
    LE_ERROR("Unsupported function called");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks if specified GPIO is available. For example, GPIO01, user would
 * invoke le_gpioCfg_IsAvailable(1);
 *
 * @return
 *       true (available)/false (not)
 */
//--------------------------------------------------------------------------------------------------
bool le_gpioCfg_IsAvailable(uint32_t gpioId)
{
    LE_ERROR("Unsupported function called");
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get list of supported GPIOs. If GPIO01, GPIO03 and GPIO05 are supported,
 * the returned list will look like {1, 5, 7}
 *
 * @return
 *    - LE_OK on success
 *    - LE_FAULT on failure
 *    - LE_OVERFLOW when given buffer is to small to store the whole list but it not an error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gpioCfg_GetSupportedGpioList
(
    uint32_t*         retListPtr,   ///< [OUT] User allocated buffer where free GPIO will be stored
    size_t*           retNumPtr     ///< [IN/OUT] Number of elements on the buffer.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * The place where the component starts up.  All initialization happens here.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    gpioSysfs_Design_t gpioDesign = SYSFS_GPIO_DESIGN_V1;

    gpioSysfs_Initialize(&gpioDesign);

    if (SYSFS_GPIO_DESIGN_V2 == gpioDesign)
    {
        SysfsGpioPin1.gpioName = "1";
        SysfsGpioPin2.gpioName = "2";
        SysfsGpioPin3.gpioName = "3";
        SysfsGpioPin4.gpioName = "4";
        SysfsGpioPin5.gpioName = "5";
        SysfsGpioPin6.gpioName = "6";
        SysfsGpioPin7.gpioName = "7";
        SysfsGpioPin8.gpioName = "8";
        SysfsGpioPin9.gpioName = "9";
        SysfsGpioPin10.gpioName = "10";
        SysfsGpioPin11.gpioName = "11";
        SysfsGpioPin12.gpioName = "12";
        SysfsGpioPin13.gpioName = "13";
        SysfsGpioPin14.gpioName = "14";
        SysfsGpioPin15.gpioName = "15";
        SysfsGpioPin16.gpioName = "16";
        SysfsGpioPin17.gpioName = "17";
        SysfsGpioPin18.gpioName = "18";
        SysfsGpioPin19.gpioName = "19";
        SysfsGpioPin20.gpioName = "20";
        SysfsGpioPin21.gpioName = "21";
        SysfsGpioPin22.gpioName = "22";
        SysfsGpioPin23.gpioName = "23";
        SysfsGpioPin24.gpioName = "24";
        SysfsGpioPin25.gpioName = "25";
        SysfsGpioPin26.gpioName = "26";
        SysfsGpioPin27.gpioName = "27";
        SysfsGpioPin28.gpioName = "28";
        SysfsGpioPin29.gpioName = "29";
        SysfsGpioPin30.gpioName = "30";
        SysfsGpioPin31.gpioName = "31";
        SysfsGpioPin32.gpioName = "32";
        SysfsGpioPin33.gpioName = "33";
        SysfsGpioPin34.gpioName = "34";
        SysfsGpioPin35.gpioName = "35";
        SysfsGpioPin36.gpioName = "36";
        SysfsGpioPin37.gpioName = "37";
        SysfsGpioPin38.gpioName = "38";
        SysfsGpioPin39.gpioName = "39";
        SysfsGpioPin40.gpioName = "40";
        SysfsGpioPin41.gpioName = "41";
        SysfsGpioPin42.gpioName = "42";
        SysfsGpioPin43.gpioName = "43";
        SysfsGpioPin44.gpioName = "44";
        SysfsGpioPin45.gpioName = "45";
        SysfsGpioPin46.gpioName = "46";
        SysfsGpioPin47.gpioName = "47";
        SysfsGpioPin48.gpioName = "48";
        SysfsGpioPin49.gpioName = "49";
        SysfsGpioPin50.gpioName = "50";
        SysfsGpioPin51.gpioName = "51";
        SysfsGpioPin52.gpioName = "52";
        SysfsGpioPin53.gpioName = "53";
        SysfsGpioPin54.gpioName = "54";
        SysfsGpioPin55.gpioName = "55";
        SysfsGpioPin56.gpioName = "56";
        SysfsGpioPin57.gpioName = "57";
        SysfsGpioPin58.gpioName = "58";
        SysfsGpioPin59.gpioName = "59";
        SysfsGpioPin60.gpioName = "60";
        SysfsGpioPin61.gpioName = "61";
        SysfsGpioPin62.gpioName = "62";
        SysfsGpioPin63.gpioName = "63";
        SysfsGpioPin64.gpioName = "64";
    }

    // Create my service: gpio pin1.
    if (gpioSysfs_IsPinAvailable(1) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/1", false))
    {
        LE_INFO("Starting GPIO Service for Pin 1");
        le_gpioPin1_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin1_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin1);
        le_msg_AddServiceCloseHandler(le_gpioPin1_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin1);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 1 - pin not available or disabled by config");
    }

    // Create my service: gpio pin2.
    if (gpioSysfs_IsPinAvailable(2) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/2", false))
    {
        LE_INFO("Starting GPIO Service for Pin 2");
        le_gpioPin2_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin2_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin2);
        le_msg_AddServiceCloseHandler(le_gpioPin2_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin2);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 2 - pin not available or disabled by config");
    }

    // Create my service: gpio pin3.
    if (gpioSysfs_IsPinAvailable(3) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/3", false))
    {
        LE_INFO("Starting GPIO Service for Pin 3");
        le_gpioPin3_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin3_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin3);
        le_msg_AddServiceCloseHandler(le_gpioPin3_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin3);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 3 - pin not available or disabled by config");
    }

    // Create my service: gpio pin4.
    if (gpioSysfs_IsPinAvailable(4) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/4", false))
    {
        LE_INFO("Starting GPIO Service for Pin 4");
        le_gpioPin4_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin4_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin4);
        le_msg_AddServiceCloseHandler(le_gpioPin4_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin4);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 4 - pin not available or disabled by config");
    }

    // Create my service: gpio pin5.
    if (gpioSysfs_IsPinAvailable(5) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/5", false))
    {
        LE_INFO("Starting GPIO Service for Pin 5");
        le_gpioPin5_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin5_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin5);
        le_msg_AddServiceCloseHandler(le_gpioPin5_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin5);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 5 - pin not available or disabled by config");
    }

    // Create my service: gpio pin6.
    if (gpioSysfs_IsPinAvailable(6) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/6", false))
    {
        LE_INFO("Starting GPIO Service for Pin 6");
        le_gpioPin6_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin6_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin6);
        le_msg_AddServiceCloseHandler(le_gpioPin6_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin6);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 6 - pin not available or disabled by config");
    }

    // Create my service: gpio pin7.
    if (gpioSysfs_IsPinAvailable(7) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/7", false))
    {
        LE_INFO("Starting GPIO Service for Pin 7");
        le_gpioPin7_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin7_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin7);
        le_msg_AddServiceCloseHandler(le_gpioPin7_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin7);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 7 - pin not available or disabled by config");
    }

    // Create my service: gpio pin8.
    if (gpioSysfs_IsPinAvailable(8) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/8", false))
    {
        LE_INFO("Starting GPIO Service for Pin 8");
        le_gpioPin8_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin8_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin8);
        le_msg_AddServiceCloseHandler(le_gpioPin8_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin8);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 8 - pin not available or disabled by config");
    }

    // Create my service: gpio pin9.
    if (gpioSysfs_IsPinAvailable(9) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/9", false))
    {
        LE_INFO("Starting GPIO Service for Pin 9");
        le_gpioPin9_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin9_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin9);
        le_msg_AddServiceCloseHandler(le_gpioPin9_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin9);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 9 - pin not available or disabled by config");
    }

    // Create my service: gpio pin10.
    if (gpioSysfs_IsPinAvailable(10) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/10", false))
    {
        LE_INFO("Starting GPIO Service for Pin 10");
        le_gpioPin10_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin10_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin10);
        le_msg_AddServiceCloseHandler(le_gpioPin10_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin10);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 10 - pin not available or disabled by config");
    }

    // Create my service: gpio pin11.
    if (gpioSysfs_IsPinAvailable(11) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/11", false))
    {
        LE_INFO("Starting GPIO Service for Pin 11");
        le_gpioPin11_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin11_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin11);
        le_msg_AddServiceCloseHandler(le_gpioPin11_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin11);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 11 - pin not available or disabled by config");
    }

    // Create my service: gpio pin12.
    if (gpioSysfs_IsPinAvailable(12) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/12", false))
    {
        LE_INFO("Starting GPIO Service for Pin 12");
        le_gpioPin12_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin12_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin12);
        le_msg_AddServiceCloseHandler(le_gpioPin12_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin12);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 12 - pin not available or disabled by config");
    }

    // Create my service: gpio pin13.
    if (gpioSysfs_IsPinAvailable(13) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/13", false))
    {
        LE_INFO("Starting GPIO Service for Pin 13");
        le_gpioPin13_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin13_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin13);
        le_msg_AddServiceCloseHandler(le_gpioPin13_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin13);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 13 - pin not available or disabled by config");
    }

    // Create my service: gpio pin14.
    if (gpioSysfs_IsPinAvailable(14) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/14", false))
    {
        LE_INFO("Starting GPIO Service for Pin 14");
        le_gpioPin14_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin14_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin14);
        le_msg_AddServiceCloseHandler(le_gpioPin14_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin14);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 14 - pin not available or disabled by config");
    }

    // Create my service: gpio pin15.
    if (gpioSysfs_IsPinAvailable(15) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/15", false))
    {
        LE_INFO("Starting GPIO Service for Pin 15");
        le_gpioPin15_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin15_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin15);
        le_msg_AddServiceCloseHandler(le_gpioPin15_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin15);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 15 - pin not available or disabled by config");
    }

    // Create my service: gpio pin16.
    if (gpioSysfs_IsPinAvailable(16) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/16", false))
    {
        LE_INFO("Starting GPIO Service for Pin 16");
        le_gpioPin16_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin16_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin16);
        le_msg_AddServiceCloseHandler(le_gpioPin16_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin16);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 16 - pin not available or disabled by config");
    }

    // Create my service: gpio pin17.
    if (gpioSysfs_IsPinAvailable(17) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/17", false))
    {
        LE_INFO("Starting GPIO Service for Pin 17");
        le_gpioPin17_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin17_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin17);
        le_msg_AddServiceCloseHandler(le_gpioPin17_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin17);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 17 - pin not available or disabled by config");
    }

    // Create my service: gpio pin18.
    if (gpioSysfs_IsPinAvailable(18) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/18", false))
    {
        LE_INFO("Starting GPIO Service for Pin 18");
        le_gpioPin18_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin18_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin18);
        le_msg_AddServiceCloseHandler(le_gpioPin18_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin18);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 18 - pin not available or disabled by config");
    }

    // Create my service: gpio pin19.
    if (gpioSysfs_IsPinAvailable(19) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/19", false))
    {
        LE_INFO("Starting GPIO Service for Pin 19");
        le_gpioPin19_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin19_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin19);
        le_msg_AddServiceCloseHandler(le_gpioPin19_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin19);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 19 - pin not available or disabled by config");
    }

    // Create my service: gpio pin20.
    if (gpioSysfs_IsPinAvailable(20) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/20", false))
    {
        LE_INFO("Starting GPIO Service for Pin 20");
        le_gpioPin20_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin20_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin20);
        le_msg_AddServiceCloseHandler(le_gpioPin20_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin20);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 20 - pin not available or disabled by config");
    }

    // Create my service: gpio pin21.
    if (gpioSysfs_IsPinAvailable(21) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/21", false))
    {
        LE_INFO("Starting GPIO Service for Pin 21");
        le_gpioPin21_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin21_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin21);
        le_msg_AddServiceCloseHandler(le_gpioPin21_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin21);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 21 - pin not available or disabled by config");
    }

    // Create my service: gpio pin22.
    if (gpioSysfs_IsPinAvailable(22) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/22", false))
    {
        LE_INFO("Starting GPIO Service for Pin 22");
        le_gpioPin22_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin22_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin22);
        le_msg_AddServiceCloseHandler(le_gpioPin22_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin22);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 22 - pin not available or disabled by config");
    }

    // Create my service: gpio pin23.
    if (gpioSysfs_IsPinAvailable(23) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/23", false))
    {
        LE_INFO("Starting GPIO Service for Pin 23");
        le_gpioPin23_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin23_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin23);
        le_msg_AddServiceCloseHandler(le_gpioPin23_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin23);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 23 - pin not available or disabled by config");
    }

    // Create my service: gpio pin24.
    if (gpioSysfs_IsPinAvailable(24) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/24", false))
    {
        LE_INFO("Starting GPIO Service for Pin 24");
        le_gpioPin24_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin24_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin24);
        le_msg_AddServiceCloseHandler(le_gpioPin24_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin24);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 24 - pin not available or disabled by config");
    }

    // Create my service: gpio pin25.
    if (gpioSysfs_IsPinAvailable(25) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/25", false))
    {
        LE_INFO("Starting GPIO Service for Pin 25");
        le_gpioPin25_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin25_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin25);
        le_msg_AddServiceCloseHandler(le_gpioPin25_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin25);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 25 - pin not available or disabled by config");
    }

    // Create my service: gpio pin26.
    if (gpioSysfs_IsPinAvailable(26) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/26", false))
    {
        LE_INFO("Starting GPIO Service for Pin 26");
        le_gpioPin26_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin26_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin26);
        le_msg_AddServiceCloseHandler(le_gpioPin26_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin26);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 26 - pin not available or disabled by config");
    }

    // Create my service: gpio pin27.
    if (gpioSysfs_IsPinAvailable(27) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/27", false))
    {
        LE_INFO("Starting GPIO Service for Pin 27");
        le_gpioPin27_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin27_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin27);
        le_msg_AddServiceCloseHandler(le_gpioPin27_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin27);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 27 - pin not available or disabled by config");
    }

    // Create my service: gpio pin28.
    if (gpioSysfs_IsPinAvailable(28) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/28", false))
    {
        LE_INFO("Starting GPIO Service for Pin 28");
        le_gpioPin28_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin28_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin28);
        le_msg_AddServiceCloseHandler(le_gpioPin28_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin28);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 28 - pin not available or disabled by config");
    }

    // Create my service: gpio pin29.
    if (gpioSysfs_IsPinAvailable(29) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/29", false))
    {
        LE_INFO("Starting GPIO Service for Pin 29");
        le_gpioPin29_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin29_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin29);
        le_msg_AddServiceCloseHandler(le_gpioPin29_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin29);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 29 - pin not available or disabled by config");
    }

    // Create my service: gpio pin30.
    if (gpioSysfs_IsPinAvailable(30) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/30", false))
    {
        LE_INFO("Starting GPIO Service for Pin 30");
        le_gpioPin30_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin30_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin30);
        le_msg_AddServiceCloseHandler(le_gpioPin30_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin30);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 30 - pin not available or disabled by config");
    }

    // Create my service: gpio pin31.
    if (gpioSysfs_IsPinAvailable(31) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/31", false))
    {
        LE_INFO("Starting GPIO Service for Pin 31");
        le_gpioPin31_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin31_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin31);
        le_msg_AddServiceCloseHandler(le_gpioPin31_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin31);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 31 - pin not available or disabled by config");
    }

    // Create my service: gpio pin32.
    if (gpioSysfs_IsPinAvailable(32) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/32", false))
    {
        LE_INFO("Starting GPIO Service for Pin 32");
        le_gpioPin32_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin32_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin32);
        le_msg_AddServiceCloseHandler(le_gpioPin32_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin32);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 32 - pin not available or disabled by config");
    }

    // Create my service: gpio pin33.
    if (gpioSysfs_IsPinAvailable(33) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/33", false))
    {
        LE_INFO("Starting GPIO Service for Pin 33");
        le_gpioPin33_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin33_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin33);
        le_msg_AddServiceCloseHandler(le_gpioPin33_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin33);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 33 - pin not available or disabled by config");
    }

    // Create my service: gpio pin34.
    if (gpioSysfs_IsPinAvailable(34) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/34", false))
    {
        LE_INFO("Starting GPIO Service for Pin 34");
        le_gpioPin34_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin34_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin34);
        le_msg_AddServiceCloseHandler(le_gpioPin34_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin34);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 34 - pin not available or disabled by config");
    }

    // Create my service: gpio pin35.
    if (gpioSysfs_IsPinAvailable(35) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/35", false))
    {
        LE_INFO("Starting GPIO Service for Pin 35");
        le_gpioPin35_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin35_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin35);
        le_msg_AddServiceCloseHandler(le_gpioPin35_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin35);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 35 - pin not available or disabled by config");
    }

    // Create my service: gpio pin36.
    if (gpioSysfs_IsPinAvailable(36) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/36", false))
    {
        LE_INFO("Starting GPIO Service for Pin 36");
        le_gpioPin36_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin36_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin36);
        le_msg_AddServiceCloseHandler(le_gpioPin36_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin36);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 36 - pin not available or disabled by config");
    }

    // Create my service: gpio pin37.
    if (gpioSysfs_IsPinAvailable(37) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/37", false))
    {
        LE_INFO("Starting GPIO Service for Pin 37");
        le_gpioPin37_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin37_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin37);
        le_msg_AddServiceCloseHandler(le_gpioPin37_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin37);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 37 - pin not available or disabled by config");
    }

    // Create my service: gpio pin38.
    if (gpioSysfs_IsPinAvailable(38) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/38", false))
    {
        LE_INFO("Starting GPIO Service for Pin 38");
        le_gpioPin38_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin38_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin38);
        le_msg_AddServiceCloseHandler(le_gpioPin38_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin38);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 38 - pin not available or disabled by config");
    }

    // Create my service: gpio pin39.
    if (gpioSysfs_IsPinAvailable(39) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/39", false))
    {
        LE_INFO("Starting GPIO Service for Pin 39");
        le_gpioPin39_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin39_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin39);
        le_msg_AddServiceCloseHandler(le_gpioPin39_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin39);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 39 - pin not available or disabled by config");
    }

    // Create my service: gpio pin40.
    if (gpioSysfs_IsPinAvailable(40) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/40", false))
    {
        LE_INFO("Starting GPIO Service for Pin 40");
        le_gpioPin40_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin40_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin40);
        le_msg_AddServiceCloseHandler(le_gpioPin40_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin40);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 40 - pin not available or disabled by config");
    }

    // Create my service: gpio pin41.
    if (gpioSysfs_IsPinAvailable(41) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/41", false))
    {
        LE_INFO("Starting GPIO Service for Pin 41");
        le_gpioPin41_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin41_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin41);
        le_msg_AddServiceCloseHandler(le_gpioPin41_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin41);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 41 - pin not available or disabled by config");
    }

    // Create my service: gpio pin42.
    if (gpioSysfs_IsPinAvailable(42) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/42", false))
    {
        LE_INFO("Starting GPIO Service for Pin 42");
        le_gpioPin42_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin42_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin42);
        le_msg_AddServiceCloseHandler(le_gpioPin42_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin42);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 42 - pin not available or disabled by config");
    }

    // Create my service: gpio pin43.
    if (gpioSysfs_IsPinAvailable(43) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/43", false))
    {
        LE_INFO("Starting GPIO Service for Pin 43");
        le_gpioPin43_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin43_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin43);
        le_msg_AddServiceCloseHandler(le_gpioPin43_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin43);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 43 - pin not available or disabled by config");
    }

    // Create my service: gpio pin44.
    if (gpioSysfs_IsPinAvailable(44) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/44", false))
    {
        LE_INFO("Starting GPIO Service for Pin 44");
        le_gpioPin44_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin44_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin44);
        le_msg_AddServiceCloseHandler(le_gpioPin44_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin44);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 44 - pin not available or disabled by config");
    }

    // Create my service: gpio pin45.
    if (gpioSysfs_IsPinAvailable(45) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/45", false))
    {
        LE_INFO("Starting GPIO Service for Pin 45");
        le_gpioPin45_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin45_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin45);
        le_msg_AddServiceCloseHandler(le_gpioPin45_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin45);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 45 - pin not available or disabled by config");
    }

    // Create my service: gpio pin46.
    if (gpioSysfs_IsPinAvailable(46) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/46", false))
    {
        LE_INFO("Starting GPIO Service for Pin 46");
        le_gpioPin46_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin46_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin46);
        le_msg_AddServiceCloseHandler(le_gpioPin46_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin46);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 46 - pin not available or disabled by config");
    }

    // Create my service: gpio pin47.
    if (gpioSysfs_IsPinAvailable(47) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/47", false))
    {
        LE_INFO("Starting GPIO Service for Pin 47");
        le_gpioPin47_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin47_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin47);
        le_msg_AddServiceCloseHandler(le_gpioPin47_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin47);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 47 - pin not available or disabled by config");
    }

    // Create my service: gpio pin48.
    if (gpioSysfs_IsPinAvailable(48) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/48", false))
    {
        LE_INFO("Starting GPIO Service for Pin 48");
        le_gpioPin48_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin48_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin48);
        le_msg_AddServiceCloseHandler(le_gpioPin48_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin48);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 48 - pin not available or disabled by config");
    }

    // Create my service: gpio pin49.
    if (gpioSysfs_IsPinAvailable(49) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/49", false))
    {
        LE_INFO("Starting GPIO Service for Pin 49");
        le_gpioPin49_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin49_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin49);
        le_msg_AddServiceCloseHandler(le_gpioPin49_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin49);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 49 - pin not available or disabled by config");
    }

    // Create my service: gpio pin50.
    if (gpioSysfs_IsPinAvailable(50) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/50", false))
    {
        LE_INFO("Starting GPIO Service for Pin 50");
        le_gpioPin50_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin50_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin50);
        le_msg_AddServiceCloseHandler(le_gpioPin50_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin50);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 50 - pin not available or disabled by config");
    }

    // Create my service: gpio pin51.
    if (gpioSysfs_IsPinAvailable(51) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/51", false))
    {
        LE_INFO("Starting GPIO Service for Pin 51");
        le_gpioPin51_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin51_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin51);
        le_msg_AddServiceCloseHandler(le_gpioPin51_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin51);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 51 - pin not available or disabled by config");
    }

    // Create my service: gpio pin52.
    if (gpioSysfs_IsPinAvailable(52) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/52", false))
    {
        LE_INFO("Starting GPIO Service for Pin 52");
        le_gpioPin52_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin52_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin52);
        le_msg_AddServiceCloseHandler(le_gpioPin52_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin52);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 52 - pin not available or disabled by config");
    }

    // Create my service: gpio pin53.
    if (gpioSysfs_IsPinAvailable(53) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/53", false))
    {
        LE_INFO("Starting GPIO Service for Pin 53");
        le_gpioPin53_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin53_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin53);
        le_msg_AddServiceCloseHandler(le_gpioPin53_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin53);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 53 - pin not available or disabled by config");
    }

    // Create my service: gpio pin54.
    if (gpioSysfs_IsPinAvailable(54) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/54", false))
    {
        LE_INFO("Starting GPIO Service for Pin 54");
        le_gpioPin54_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin54_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin54);
        le_msg_AddServiceCloseHandler(le_gpioPin54_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin54);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 54 - pin not available or disabled by config");
    }

    // Create my service: gpio pin55.
    if (gpioSysfs_IsPinAvailable(55) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/55", false))
    {
        LE_INFO("Starting GPIO Service for Pin 55");
        le_gpioPin55_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin55_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin55);
        le_msg_AddServiceCloseHandler(le_gpioPin55_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin55);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 55 - pin not available or disabled by config");
    }

    // Create my service: gpio pin56.
    if (gpioSysfs_IsPinAvailable(56) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/56", false))
    {
        LE_INFO("Starting GPIO Service for Pin 56");
        le_gpioPin56_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin56_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin56);
        le_msg_AddServiceCloseHandler(le_gpioPin56_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin56);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 56 - pin not available or disabled by config");
    }

    // Create my service: gpio pin57.
    if (gpioSysfs_IsPinAvailable(57) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/57", false))
    {
        LE_INFO("Starting GPIO Service for Pin 57");
        le_gpioPin57_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin57_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin57);
        le_msg_AddServiceCloseHandler(le_gpioPin57_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin57);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 57 - pin not available or disabled by config");
    }

    // Create my service: gpio pin58.
    if (gpioSysfs_IsPinAvailable(58) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/58", false))
    {
        LE_INFO("Starting GPIO Service for Pin 58");
        le_gpioPin58_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin58_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin58);
        le_msg_AddServiceCloseHandler(le_gpioPin58_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin58);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 58 - pin not available or disabled by config");
    }

    // Create my service: gpio pin59.
    if (gpioSysfs_IsPinAvailable(59) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/59", false))
    {
        LE_INFO("Starting GPIO Service for Pin 59");
        le_gpioPin59_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin59_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin59);
        le_msg_AddServiceCloseHandler(le_gpioPin59_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin59);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 59 - pin not available or disabled by config");
    }

    // Create my service: gpio pin60.
    if (gpioSysfs_IsPinAvailable(60) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/60", false))
    {
        LE_INFO("Starting GPIO Service for Pin 60");
        le_gpioPin60_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin60_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin60);
        le_msg_AddServiceCloseHandler(le_gpioPin60_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin60);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 60 - pin not available or disabled by config");
    }

    // Create my service: gpio pin61.
    if (gpioSysfs_IsPinAvailable(61) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/61", false))
    {
        LE_INFO("Starting GPIO Service for Pin 61");
        le_gpioPin61_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin61_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin61);
        le_msg_AddServiceCloseHandler(le_gpioPin61_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin61);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 61 - pin not available or disabled by config");
    }

    // Create my service: gpio pin62.
    if (gpioSysfs_IsPinAvailable(62) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/62", false))
    {
        LE_INFO("Starting GPIO Service for Pin 62");
        le_gpioPin62_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin62_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin62);
        le_msg_AddServiceCloseHandler(le_gpioPin62_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin62);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 62 - pin not available or disabled by config");
    }

    // Create my service: gpio pin63.
    if (gpioSysfs_IsPinAvailable(63) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/63", false))
    {
        LE_INFO("Starting GPIO Service for Pin 63");
        le_gpioPin63_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin63_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin63);
        le_msg_AddServiceCloseHandler(le_gpioPin63_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin63);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 63 - pin not available or disabled by config");
    }

    // Create my service: gpio pin64.
    if (gpioSysfs_IsPinAvailable(64) && !le_cfg_QuickGetBool("gpioService:/pins/disabled/64", false))
    {
        LE_INFO("Starting GPIO Service for Pin 64");
        le_gpioPin64_AdvertiseService();
        le_msg_AddServiceOpenHandler(le_gpioPin64_GetServiceRef(),
                                     gpioSysfs_SessionOpenHandlerFunc,
                                     (void*)gpioRefPin64);
        le_msg_AddServiceCloseHandler(le_gpioPin64_GetServiceRef(),
                                      gpioSysfs_SessionCloseHandlerFunc,
                                      (void*)gpioRefPin64);
    }
    else
    {
        LE_INFO("Skipping starting GPIO Service for Pin 64 - pin not available or disabled by config");
    }

    // Begin monitoring main event loop
    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);
}

