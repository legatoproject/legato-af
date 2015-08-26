/**
 * @page c_pa_temp Temperature Monitoring Platform Adapter API
 *
 * @ref pa_temp.h "API Reference"
 *
 * <HR>
 *
 * @section pa_temp_toc Table of Contents
 *
 *  - @ref pa_temp_intro
 *  - @ref pa_temp_rational
 *
 *
 * @section pa_temp_intro Introduction
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developped upon these APIs.
 *
 *
 * @section pa_temp_rational Rational
 * These functions are all blocking functions, so that they return when the modem has answered or
 * when a timeout has occured due to an interrupted communication with the modem.
 *
 * They all verify the validity and the range of the input parameters before performing the modem
 * operation.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


/** @file pa_temp.h
 *
 * Legato @ref c_pa_temp include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PATEMP_INCLUDE_GUARD
#define LEGATO_PATEMP_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report the Radio Access Technology change.
 *
 * @param ratPtr The Radio Access Technology.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_temp_ThresholdInd_HandlerFunc_t)
(
                le_temp_ThresholdStatus_t* thresholdEventPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Radio temperature level in degree celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the temperature.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_temp_GetRadioTemperature
(
    int32_t* radioTempPtr
        ///< [OUT]
        ///< [OUT] The Radio temperature level in degree celsius.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform temperature level in degree celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the temperature.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_temp_GetPlatformTemperature
(
    int32_t* platformTempPtr
        ///< [OUT]
        ///< [OUT] The Platform temperature level in degree celsius.
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to add a temperature status notification handler
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_event_HandlerRef_t* pa_temp_AddTempEventHandler
(
    pa_temp_ThresholdInd_HandlerFunc_t   msgHandler
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the Radio warning and critical temperature thresholds in degree celsius.
 *  When thresholds temperature are reached, a temperature event is triggered.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to set the thresholds.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_temp_SetRadioThresholds
(
    int32_t hiWarningTemp,
        ///< [IN]
        ///< [IN] The high warning temperature threshold in degree celsius.

    int32_t hiCriticalTemp
        ///< [IN]
        ///< [IN] The high critical temperature threshold in degree celsius.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Radio warning and critical temperature thresholds in degree celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the thresholds.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_temp_GetRadioThresholds
(
    int32_t* hiWarningTempPtr,
        ///< [OUT]
        ///< [OUT] The high warning temperature threshold in degree celsius.

    int32_t* hiCriticalTempPtr
        ///< [OUT]
        ///< [OUT] The high critical temperature threshold
        ///<  in degree celsius.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the Platform warning and critical temperature thresholds in degree celsius.
 *  When thresholds temperature are reached, a temperature event is triggered.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to set the thresholds.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_temp_SetPlatformThresholds
(
    int32_t lowCriticalTemp,
        ///< [IN]
        ///< [IN] The low critical temperature threshold in degree celsius.

    int32_t lowWarningTemp,
        ///< [IN]
        ///< [IN] The low warning temperature threshold in degree celsius.

    int32_t hiWarningTemp,
        ///< [IN]
        ///< [IN] The high warning temperature threshold in degree celsius.

    int32_t hiCriticalTemp
        ///< [IN]
        ///< [IN] The high critical temperature threshold in degree celsius.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform warning and critical temperature thresholds in degree celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the thresholds.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_temp_GetPlatformThresholds
(
    int32_t* lowCriticalTempPtr,
        ///< [OUT]
        ///< [OUT] The low critical temperature threshold in degree celsius.

    int32_t* lowWarningTempPtr,
        ///< [OUT]
        ///< [OUT] The low warning temperature threshold in degree celsius.

    int32_t* hiWarningTempPtr,
        ///< [OUT]
        ///< [OUT] The high warning temperature threshold in degree celsius.

    int32_t* hiCriticalTempPtr
        ///< [OUT]
        ///< [OUT] The high critical temperature threshold
        ///<  in degree celsius.
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to intialize the PA Temperature
 *
 * @return
 * - LE_OK if successful.
 * - LE_FAULT if unsuccessful.
 *
 * @note This function should not be called from outside the platform adapter.
 *
 * @todo Move this prototype to another (internal) header.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_Init
(
    void
);

#endif // LEGATO_PATEMP_INCLUDE_GUARD
