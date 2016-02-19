/**
 * @file pa_temp.c
 *
 * AT implementation of Temperature API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_temp.h"

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Get the Radio temperature level in degree celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the temperature.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_GetRadioTemperature
(
    int32_t* radioTempPtr
        ///< [OUT]
        ///< [OUT] The Radio temperature level in degree celsius.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform temperature level in degree celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the temperature.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_GetPlatformTemperature
(
    int32_t* platformTempPtr
        ///< [OUT]
        ///< [OUT] The Platform temperature level in degree celsius.
)
{
    return LE_FAULT;
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
    pa_temp_ThresholdInd_HandlerFunc_t   msgHandler
)
{
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the Radio warning and critical temperature thresholds in degree celsius.
 *  When thresholds temperature are reached, a temperature event is triggered.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_BAD_PARAMETER The hiWarning threshold + 1 is equal to or higher than
 *                           the hiCritical threshold.
 *      - LE_FAULT         The function failed to set the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_SetRadioThresholds
(
    int32_t hiWarningTemp,
        ///< [IN]
        ///< [IN] The high warning temperature threshold in degree celsius.

    int32_t hiCriticalTemp
        ///< [IN]
        ///< [IN] The high critical temperature threshold in degree celsius.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Radio warning and critical temperature thresholds in degree celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_GetRadioThresholds
(
    int32_t* hiWarningTempPtr,
        ///< [OUT]
        ///< [OUT] The high warning temperature threshold in degree celsius.

    int32_t* hiCriticalTempPtr
        ///< [OUT]
        ///< [OUT] The high critical temperature threshold
        ///<  in degree celsius.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Platform warning and critical temperature thresholds in degree celsius.
 *  When thresholds temperature are reached, a temperature event is triggered.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_BAD_PARAMETER The hiWarning threshold + 1 is equal to or higher than
 *                           the hiCritical threshold.
 *                         The loWwarning threshold is equal to or higher than
 *                           the hiWarning threshold.
 *                         The loWwarning threshold is equal to or lower than
 *                           the loCritical threshold.
 *      - LE_FAULT         The function failed to set the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_SetPlatformThresholds
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
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform warning and critical temperature thresholds in degree celsius.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_temp_GetPlatformThresholds
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
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to intialize the PA Temperature
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
    return LE_OK;
}
