//--------------------------------------------------------------------------------------------------
/**
 * @file le_ips.c
 *
 * This file contains the source code of the Input Power Supply Monitoring API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pa_ips.h"

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New input voltage Threshold event notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t VoltageThresholdEventId;

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Input Voltage Change Handler.
 *
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
 *
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ips_GetInputVoltage
(
    uint32_t* inputVoltagePtr
        ///< [OUT]
        ///< [OUT] The input voltage in [mV]
)
{
    if (inputVoltagePtr == NULL)
    {
        LE_KILL_CLIENT("inputVoltagePtr is NULL!");
        return LE_FAULT;
    }

    return  pa_ips_GetInputVoltage(inputVoltagePtr);
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
    uint16_t criticalVolt,
        ///< [IN]
        ///< [IN] The critical input voltage threshold in [mV].

    uint16_t warningVolt,
        ///< [IN]
        ///< [IN] The warning input voltage threshold in [mV].

    uint16_t normalVolt,
        ///< [IN]
        ///< [IN] The normal input voltage threshold in [mV].

    uint16_t hiCriticalVolt
        ///< [IN]
        ///< [IN] The high critical input voltage threshold in [mV].
)
{
    if ((criticalVolt >= warningVolt) || (warningVolt >= normalVolt)
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ips_GetVoltageThresholds
(
    uint16_t* criticalVoltPtr,
        ///< [OUT]
        ///< [OUT] The critical input voltage threshold in [mV].

    uint16_t* warningVoltPtr,
        ///< [OUT]
        ///< [OUT] The warning input voltage threshold in [mV].

    uint16_t* normalVoltPtr,
        ///< [OUT]
        ///< [OUT] The normal input voltage threshold in [mV].

    uint16_t* hiCriticalVoltPtr
        ///< [OUT]
        ///< [IN] The high critical input voltage threshold in [mV].
)
{
    if ((criticalVoltPtr == NULL) || (warningVoltPtr == NULL)
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
    le_ips_ThresholdEventHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
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
    le_ips_ThresholdEventHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    if (addHandlerRef == NULL)
    {
        LE_KILL_CLIENT("addHandlerRef function is NULL !");
    }

    le_event_RemoveHandler((le_event_HandlerRef_t) addHandlerRef);
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
