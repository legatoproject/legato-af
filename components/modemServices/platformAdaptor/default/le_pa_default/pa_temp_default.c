/**
 * @file pa_temp_default.c
 *
 * Default implementation of @ref c_pa_temp.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_temp.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to initialize the PA Temperature
 *
 * @return
 * - LE_OK if successful.
 * - LE_FAULT if unsuccessful.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_temp_Init
(
    void
)
{
    return LE_OK;
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
le_result_t pa_temp_GetSensorName
(
    pa_temp_Handle_t    paHandle, ///< [IN] Handle of the temperature sensor.
    char*               namePtr,  ///< [OUT] Name of the temperature sensor.
    size_t              len       ///< [IN] The maximum length of the sensor name.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
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
    LE_ERROR("Unsupported function called");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset the temperature sensor handle.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed.
 *      - LE_UNSUPPORTED   The function does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_ResetHandle
(
    const char*         sensorPtr  ///< [IN] Name of the temperature sensor.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}
