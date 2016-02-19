/**
 * @file pa_temp_simu.c
 *
 * SIMU implementation of Temperature API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_temp.h"
#include "pa_temp_simu.h"

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Storage structure for radio thresholds.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    int32_t hiWarningTemp;  ///< The high warning temperature threshold in degree celsius.
    int32_t hiCriticalTemp; ///< The high critical temperature threshold in degree celsius.
}
RadioThresholds_t;

//--------------------------------------------------------------------------------------------------
/**
 * Storage structure for platform thresholds.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    int32_t lowWarningTemp;  ///< The low warning temperature threshold in degree celsius.
    int32_t hiWarningTemp;   ///< The high warning temperature threshold in degree celsius.
    int32_t lowCriticalTemp; ///< The low critical temperature threshold in degree celsius.
    int32_t hiCriticalTemp;  ///< The high critical temperature threshold in degree celsius.
}
PlatformThresholds_t;

//--------------------------------------------------------------------------------------------------
/**
 * Storage structure for thresholds.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    RadioThresholds_t radio;
    PlatformThresholds_t platform;
}
Thresholds_t;

//--------------------------------------------------------------------------------------------------
/**
 * Global structure to store active thresholds.
 */
//--------------------------------------------------------------------------------------------------
static Thresholds_t Thresholds = {
    .radio = {
        .hiWarningTemp = PA_SIMU_TEMP_DEFAULT_RADIO_HIGH_WARN,
        .hiCriticalTemp = PA_SIMU_TEMP_DEFAULT_RADIO_HIGH_CRIT,
    },
    .platform = {
        .lowWarningTemp = PA_SIMU_TEMP_DEFAULT_PLATFORM_LOW_WARN,
        .hiWarningTemp = PA_SIMU_TEMP_DEFAULT_PLATFORM_HIGH_WARN,
        .lowCriticalTemp = PA_SIMU_TEMP_DEFAULT_PLATFORM_LOW_CRIT,
        .hiCriticalTemp = PA_SIMU_TEMP_DEFAULT_PLATFORM_HIGH_CRIT,
    },
};

static le_result_t ReturnCode = LE_OK;
static le_event_Id_t TemperatureThresholdEventId;
static le_mem_PoolRef_t TemperatureThresholdEventPool;

//--------------------------------------------------------------------------------------------------
/**
 * Set the stub return code.
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
    le_temp_ThresholdStatus_t status
)
{
    le_temp_ThresholdStatus_t * tempEventPtr = le_mem_ForceAlloc(TemperatureThresholdEventPool);

    *tempEventPtr = status;

    le_event_ReportWithRefCounting(TemperatureThresholdEventId, tempEventPtr);
}

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
    if (ReturnCode == LE_OK)
    {
        *radioTempPtr = PA_SIMU_TEMP_DEFAULT_RADIO_TEMP;
    }

    return ReturnCode;
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
    if (ReturnCode == LE_OK)
    {
        *platformTempPtr = PA_SIMU_TEMP_DEFAULT_PLATFORM_TEMP;
    }
    return ReturnCode;
}

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
    if (ReturnCode == LE_OK)
    {
        Thresholds.radio.hiWarningTemp = hiWarningTemp;
        Thresholds.radio.hiCriticalTemp = hiCriticalTemp;
    }
    return ReturnCode;
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
    if (ReturnCode == LE_OK)
    {
        *hiWarningTempPtr = Thresholds.radio.hiWarningTemp;
        *hiCriticalTempPtr = Thresholds.radio.hiCriticalTemp;
    }
    return ReturnCode;
}


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
    if (ReturnCode == LE_OK)
    {
        Thresholds.platform.lowWarningTemp = lowWarningTemp;
        Thresholds.platform.hiWarningTemp = hiWarningTemp;
        Thresholds.platform.lowCriticalTemp = lowCriticalTemp;
        Thresholds.platform.hiCriticalTemp = hiCriticalTemp;
    }
    return ReturnCode;
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
    if (ReturnCode == LE_OK)
    {
        *lowWarningTempPtr = Thresholds.platform.lowWarningTemp;
        *hiWarningTempPtr = Thresholds.platform.hiWarningTemp;
        *lowCriticalTempPtr = Thresholds.platform.lowCriticalTemp;
        *hiCriticalTempPtr = Thresholds.platform.hiCriticalTemp;
    }
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
    pa_temp_ThresholdInd_HandlerFunc_t   msgHandler
)
{
    le_event_HandlerRef_t  handlerRef = NULL;

    if ( msgHandler != NULL )
    {
        handlerRef = le_event_AddHandler( "ThresholdStatushandler",
                             TemperatureThresholdEventId,
                            (le_event_HandlerFunc_t) msgHandler);
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
    // Create the event for signaling user handlers.
    TemperatureThresholdEventId = le_event_CreateIdWithRefCounting("TemperatureStatusEvent");

    TemperatureThresholdEventPool = le_mem_CreatePool("TemperatureStatusEventPool",
                    sizeof(le_temp_ThresholdStatus_t));

    return LE_OK;
}
