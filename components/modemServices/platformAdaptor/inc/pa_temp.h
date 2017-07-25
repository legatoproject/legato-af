/**
 * @page c_pa_temp Temperature Monitoring Platform Adapter API
 *
 * @ref pa_temp.h "API Reference"
 *
 * <HR>
 *
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developped upon these APIs.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


/** @file pa_temp.h
 *
 * Legato @ref c_pa_temp include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PATEMP_INCLUDE_GUARD
#define LEGATO_PATEMP_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * PA Temp resource opaque handle declaration (Sensor context in PA side, opaque type in LE side)
 */
//--------------------------------------------------------------------------------------------------
typedef struct pa_temp_Handle* pa_temp_Handle_t;

//--------------------------------------------------------------------------------------------------
/**
 * LE Temp resource opaque handle declaration (Sensor context in LE side, opaque type in PA side)
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_temp_Handle* le_temp_Handle_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report a temperature threshold.
 *
 * @param leHandle      Sensor context in LE side, opaque type in PA side
 * @param thresholdPtr  Name of the threshold.
 * @param contextPtr    Context to be given to the handler.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_temp_ThresholdHandlerFunc_t)
(
    le_temp_Handle_t leHandle,
    const char*      thresholdPtr,
    void*            contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Request a new handle for a temperature sensor
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_temp_Request
(
    const char*         sensorPtr,  ///< [IN] Name of the temperature sensor.
    le_temp_Handle_t    leHandle,   ///< [IN] Sensor context in LE side, opaque type in PA side
    pa_temp_Handle_t*   paHandlePtr ///< [OUT] Sensor context in PA side, opaque type in LE side
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the handle of a temperature sensor
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_temp_GetHandle
(
    const char*         sensorPtr,  ///< [IN] Name of the temperature sensor.
    le_temp_Handle_t*   leHandlePtr ///< [OUT] Sensor context in LE side, opaque type in PA side
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the temperature in degree Celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the temperature.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_temp_GetTemperature
(
    pa_temp_Handle_t    paHandle,      ///< [IN] Handle of the temperature sensor.
    int32_t*            temperaturePtr ///< [OUT] Temperature in degree Celsius.
);

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
LE_SHARED le_result_t pa_temp_SetThreshold
(
    pa_temp_Handle_t    paHandle,      ///< [IN] Handle of the temperature sensor.
    const char*         thresholdPtr,  ///< [IN] Name of the threshold.
    int32_t             temperature    ///< [IN] Temperature threshold in degree Celsius.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Radio warning and critical temperature thresholds in degree Celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the thresholds.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_temp_GetThreshold
(
    pa_temp_Handle_t    paHandle,      ///< [IN] Handle of the temperature sensor.
    const char*         thresholdPtr,  ///< [IN] Name of the threshold.
    int32_t*            temperaturePtr ///< [OUT] Temperature in degree Celsius.
);

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
LE_SHARED le_result_t pa_temp_StartMonitoring
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to add a temperature status notification handler
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_event_HandlerRef_t* pa_temp_AddTempEventHandler
(
    pa_temp_ThresholdHandlerFunc_t  handlerFunc, ///< [IN] The handler function.
    void*                           contextPtr   ///< [IN] The context to be given to the handler.
);

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
LE_SHARED le_result_t pa_temp_ResetHandle
(
    const char*         sensorPtr  ///< [IN] Name of the temperature sensor.
);

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
);

#endif // LEGATO_PATEMP_INCLUDE_GUARD
