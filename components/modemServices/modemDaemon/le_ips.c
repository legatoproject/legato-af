//--------------------------------------------------------------------------------------------------
/**
 * @file le_ips.c
 *
 * This file contains the source code of the Input Power Supply Monitoring API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pa_ips.h"

//--------------------------------------------------------------------------------------------------
/**
 * Maximal battery level.
 */
//--------------------------------------------------------------------------------------------------
#define BATTERY_LEVEL_MAX   100

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New input voltage Threshold event notification.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t VoltageThresholdEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Battery level provided by dedicated API, set to an incorrect value by default to indicate
 * that it is not used.
 */
//--------------------------------------------------------------------------------------------------
static uint8_t ExternalBatteryLevel = BATTERY_LEVEL_MAX + 1;

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Input Voltage Change Handler.
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerVoltageChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_ips_ThresholdStatus_t*           ipsPtr = reportPtr;
    le_ips_ThresholdEventHandlerFunc_t  clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*ipsPtr, le_event_GetContextPtr());

    // The reportPtr is a reference counted object, so need to release it
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Input Voltage Change handler function.
 */
//--------------------------------------------------------------------------------------------------
static void VoltageChangeHandler
(
    le_ips_ThresholdStatus_t* thresholdEventPtr
)
{
    // Notify all the registered client's handlers
    le_event_ReportWithRefCounting(VoltageThresholdEventId, thresholdEventPtr);
}


//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform voltage input in [mV].
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ips_GetInputVoltage
(
    uint32_t* inputVoltagePtr   ///< [OUT] The input voltage in [mV]
)
{
    if (inputVoltagePtr == NULL)
    {
        LE_KILL_CLIENT("inputVoltagePtr is NULL!");
        return LE_FAULT;
    }

    return pa_ips_GetInputVoltage(inputVoltagePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Platform warning and critical input voltage thresholds in [mV].
 *  When thresholds input voltage are reached, a input voltage event is triggered.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_BAD_PARAMETER The hiCriticalVolt threshold is equal or lower than the (normalVolt+1)
 *                           threshold.
 *                         The warningVolt threshold is equal to or higher than the normalVolt
 *                           threshold.
 *                         The warningVolt threshold is equal to or lower than the criticalVolt
 *                           threshold.
 *      - LE_FAULT         The function failed to set the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ips_SetVoltageThresholds
(
    uint16_t criticalVolt,  ///< [IN] The critical input voltage threshold in [mV].
    uint16_t warningVolt,   ///< [IN] The warning input voltage threshold in [mV].
    uint16_t normalVolt,    ///< [IN] The normal input voltage threshold in [mV].
    uint16_t hiCriticalVolt ///< [IN] The high critical input voltage threshold in [mV].
)
{
    if (   (criticalVolt >= warningVolt)
        || (warningVolt >= normalVolt)
        || (hiCriticalVolt <= (normalVolt+1)))
    {
        LE_ERROR("Condition hiCriticalVolt > (normalVolt+1) or"
            " normalVolt > warningVolt > criticalVolt is FAILED");
        return LE_BAD_PARAMETER;
    }

    return pa_SetVoltageThresholds(criticalVolt, warningVolt, normalVolt, hiCriticalVolt);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform warning and critical input voltage thresholds in [mV].
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the thresholds.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ips_GetVoltageThresholds
(
    uint16_t* criticalVoltPtr,  ///< [OUT] The critical input voltage threshold in [mV].
    uint16_t* warningVoltPtr,   ///< [OUT] The warning input voltage threshold in [mV].
    uint16_t* normalVoltPtr,    ///< [OUT] The normal input voltage threshold in [mV].
    uint16_t* hiCriticalVoltPtr ///< [IN] The high critical input voltage threshold in [mV].
)
{
    if (   (criticalVoltPtr == NULL) || (warningVoltPtr == NULL)
        || (normalVoltPtr == NULL) || (hiCriticalVoltPtr == NULL))
    {
        LE_KILL_CLIENT("Parameters pointer are NULL!!");
        return LE_FAULT;
    }

    return pa_GetVoltageThresholds(criticalVoltPtr, warningVoltPtr,
                                   normalVoltPtr, hiCriticalVoltPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_ips_ThresholdEvent'
 *
 * This event provides information on Threshold reached.
 */
//--------------------------------------------------------------------------------------------------
le_ips_ThresholdEventHandlerRef_t le_ips_AddThresholdEventHandler
(
    le_ips_ThresholdEventHandlerFunc_t handlerPtr,  ///< [IN] The event handler function.
    void* contextPtr                                ///< [IN] The handler context.
)
{
    le_event_HandlerRef_t        handlerRef;

    if (handlerPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("VoltTHandler",
                                            VoltageThresholdEventId,
                                            FirstLayerVoltageChangeHandler,
                                            (le_event_HandlerFunc_t)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_ips_ThresholdEventHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_ips_ThresholdEvent'
 */
//--------------------------------------------------------------------------------------------------
void le_ips_RemoveThresholdEventHandler
(
    le_ips_ThresholdEventHandlerRef_t addHandlerRef ///< [IN] The handler object to remove.
)
{
    if (addHandlerRef == NULL)
    {
        LE_KILL_CLIENT("addHandlerRef function is NULL !");
    }

    le_event_RemoveHandler((le_event_HandlerRef_t) addHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform power source.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ips_GetPowerSource
(
    le_ips_PowerSource_t* powerSourcePtr    ///< [OUT] The power source.
)
{
    if (NULL == powerSourcePtr)
    {
        LE_KILL_CLIENT("powerSourcePtr is NULL!");
        return LE_FAULT;
    }

    // Check if a battery level was provided
    if (ExternalBatteryLevel <= BATTERY_LEVEL_MAX)
    {
        *powerSourcePtr = LE_IPS_POWER_SOURCE_BATTERY;
        return LE_OK;
    }

    // Retrieve battery level from the device otherwise
    return pa_ips_GetPowerSource(powerSourcePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform battery level in percent:
 *  - 0: battery is exhausted or platform does not have a battery connected
 *  - 1 to 100: percentage of battery capacity remaining
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ips_GetBatteryLevel
(
    uint8_t* batteryLevelPtr    ///< [OUT] The battery level in percent.
)
{
    if (NULL == batteryLevelPtr)
    {
        LE_KILL_CLIENT("batteryLevelPtr is NULL!");
        return LE_FAULT;
    }

    // Check if a battery level was provided
    if (ExternalBatteryLevel <= BATTERY_LEVEL_MAX)
    {
        *batteryLevelPtr = ExternalBatteryLevel;
        return LE_OK;
    }

    // Retrieve power source from the device otherwise
    return pa_ips_GetBatteryLevel(batteryLevelPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Platform battery level in percent.
 * This is useful when an external battery is used and its level is provided by the application
 * monitoring it.
 *
 * @note The battery level set through this API will be the value reported by
 * le_ips_GetBatteryLevel() until Legato is restarted.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_BAD_PARAMETER Incorrect battery level provided.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ips_SetBatteryLevel
(
    uint8_t batteryLevel    ///< [IN] The battery level in percent.
)
{
    if (batteryLevel > BATTERY_LEVEL_MAX)
    {
        LE_ERROR("Incorrect battery level, %d%% > %d%%", batteryLevel, BATTERY_LEVEL_MAX);
        return LE_BAD_PARAMETER;
    }

    ExternalBatteryLevel = batteryLevel;
    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Initialization of the input voltage Monitoring Service
 */
//--------------------------------------------------------------------------------------------------
void le_ips_Init
(
    void
)
{
    // Create an event Id for input voltage change notification
    VoltageThresholdEventId = le_event_CreateIdWithRefCounting("VoltageThresholdEvent");

    // Register a handler function for new input voltage Threshold Event
    pa_ips_AddVoltageEventHandler(VoltageChangeHandler);
}
