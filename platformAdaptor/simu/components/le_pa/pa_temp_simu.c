/**
 * @file pa_temp_simu.c
 *
 * SIMU implementation of @ref c_pa_temp.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_temp.h"
#include "pa_temp_simu.h"



//--------------------------------------------------------------------------------------------------
/**
 * Sensor context structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    le_temp_Handle_t    leHandle;
    char                name[LE_TEMP_SENSOR_NAME_MAX_BYTES];
    int16_t             temperature;        ///<  Temperature in degrees Celsius.
    bool                hiCriticalValid;    ///<  true if High Critical threshold is set.
    int16_t             hiCritical;         ///<  High Critical threshold.
} PaSensorContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * Temperature threshold report structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_temp_Handle_t    leHandle;
    char                event[LE_TEMP_THRESHOLD_NAME_MAX_BYTES];
} ThresholdEventReport_t;


static le_result_t          ReturnCode = LE_OK;
static le_event_Id_t        ThresholdEventId;
static le_mem_PoolRef_t     ThresholdReportPool;
static PaSensorContext_t    PaSensorHandle;


//--------------------------------------------------------------------------------------------------
/**
 * The first-layer temperature Handler.
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerTemperatureHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    ThresholdEventReport_t*             tempPtr = reportPtr;
    pa_temp_ThresholdHandlerFunc_t      clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc((le_temp_Handle_t)tempPtr->leHandle,
                      tempPtr->event,
                      le_event_GetContextPtr());

    // The reportPtr is a reference counted object, so need to release it
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Set the stubbed return code.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_tempSimu_SetReturnCode
(
    le_result_t res
)
{
    ReturnCode = res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Trigger a temperature event report.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_tempSimu_TriggerEventReport
(
    const char*  thresholdPtr  ///< [IN] Name of the threshold.
)
{
    ThresholdEventReport_t* tempEventPtr = le_mem_ForceAlloc(ThresholdReportPool);

    tempEventPtr->leHandle = PaSensorHandle.leHandle;

    strncpy(tempEventPtr->event, thresholdPtr, LE_TEMP_THRESHOLD_NAME_MAX_LEN);

    le_event_ReportWithRefCounting(ThresholdEventId, tempEventPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Request a new handle for a temperature sensor
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_Request
(
    const char*         sensorPtr,  ///< [IN] Name of the temperature sensor.
    le_temp_Handle_t    leHandle,   ///< [IN] Sensor context in LE side, opaque type in PA side
    pa_temp_Handle_t*   paHandlePtr ///< [OUT] Sensor context in PA side, opaque type in LE side
)
{
    if (ReturnCode == LE_OK)
    {
        PaSensorHandle.leHandle = leHandle;
        *paHandlePtr = (pa_temp_Handle_t)&PaSensorHandle;
    }
    return ReturnCode;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the handle of a temperature sensor
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_GetHandle
(
    const char*         sensorPtr,  ///< [IN] Name of the temperature sensor.
    le_temp_Handle_t*   leHandlePtr ///< [OUT] Sensor context in LE side, opaque type in PA side
)
{
    if (ReturnCode == LE_OK)
    {
        *leHandlePtr = PaSensorHandle.leHandle;
    }
    return ReturnCode;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the temperature sensor's name from its handle.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_OVERFLOW      The name length exceed the maximum length.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_temp_GetSensorName
(
    pa_temp_Handle_t    paHandle, ///< [IN] Handle of the temperature sensor.
    char*               namePtr,  ///< [OUT] Name of the temperature sensor.
    size_t              len       ///< [IN] The maximum length of the sensor name.
)
{
    if (ReturnCode == LE_OK)
    {
        strncpy(namePtr, PaSensorHandle.name, len);
    }
    return ReturnCode;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the temperature in degree Celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the temperature.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_GetTemperature
(
    pa_temp_Handle_t    paHandle,      ///< [IN] Handle of the temperature sensor.
    int32_t*            temperaturePtr ///< [OUT] Temperature in degree Celsius.
)
{
    PaSensorContext_t* sensorCtxPtr = (PaSensorContext_t*)paHandle;
    if (ReturnCode == LE_OK)
    {
        *temperaturePtr = sensorCtxPtr->temperature;
    }
    return ReturnCode;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Radio warning and critical temperature thresholds in degree Celsius.
 *  When thresholds temperature are reached, a temperature event is triggered.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to set the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_SetThreshold
(
    pa_temp_Handle_t    paHandle,      ///< [IN] Handle of the temperature sensor.
    const char*         thresholdPtr,  ///< [IN] Name of the threshold.
    int32_t             temperature    ///< [IN] Temperature threshold in degree Celsius.
)
{
    PaSensorContext_t* sensorCtxPtr = (PaSensorContext_t*)paHandle;
    if (ReturnCode == LE_OK)
    {
        sensorCtxPtr->hiCritical = temperature;
        sensorCtxPtr->hiCriticalValid = true;
    }
    return ReturnCode;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Radio warning and critical temperature thresholds in degree Celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_GetThreshold
(
    pa_temp_Handle_t    paHandle,      ///< [IN] Handle of the temperature sensor.
    const char*         thresholdPtr,  ///< [IN] Name of the threshold.
    int32_t*            temperaturePtr ///< [OUT] Temperature in degree Celsius.
)
{
    PaSensorContext_t* sensorCtxPtr = (PaSensorContext_t*)paHandle;
    if (ReturnCode == LE_OK)
    {
        *temperaturePtr = sensorCtxPtr->hiCritical;
    }
    return ReturnCode;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start the temperature monitoring with the temperature thresholds configured by
 * le_temp_SetThreshold() function.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to apply the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_StartMonitoring
(
    void
)
{
    return ReturnCode;
}

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to add a temperature status notification handler
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t* pa_temp_AddTempEventHandler
(
    pa_temp_ThresholdHandlerFunc_t  handlerFunc, ///< [IN] The handler function.
    void*                           contextPtr   ///< [IN] The context to be given to the handler.
)
{
    le_event_HandlerRef_t  handlerRef = NULL;

    if (handlerFunc != NULL)
    {
        handlerRef = le_event_AddLayeredHandler("ThresholdStatushandler",
                                                ThresholdEventId,
                                                FirstLayerTemperatureHandler,
                                                (le_event_HandlerFunc_t)handlerFunc);
        le_event_SetContextPtr (handlerRef, contextPtr);
    }
    else
    {
        LE_ERROR("Null handler given in parameter");
    }

    return (le_event_HandlerRef_t*) handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to initialize the PA Temperature
 *
 * @return
 * - LE_OK if successful.
 * - LE_FAULT if unsuccessful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_Init
(
    void
)
{
    // Create the event for signaling user handlers.
    ThresholdEventId = le_event_CreateIdWithRefCounting("TempThresholdsEvent");

    ThresholdReportPool = le_mem_CreatePool("ThresholdReportPool",
                                            sizeof(ThresholdEventReport_t));

    PaSensorHandle.leHandle = NULL;
    PaSensorHandle.hiCritical = PA_SIMU_TEMP_DEFAULT_HI_CRIT;
    PaSensorHandle.hiCriticalValid = false;
    PaSensorHandle.temperature = PA_SIMU_TEMP_DEFAULT_TEMPERATURE;
    strncpy(PaSensorHandle.name, PA_SIMU_TEMP_SENSOR, LE_TEMP_SENSOR_NAME_MAX_BYTES);

    return LE_OK;
}
