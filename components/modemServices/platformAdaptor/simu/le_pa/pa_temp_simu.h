/** @file pa_temp_simu.h
 *
 * Legato @ref pa_temp_simu include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef PA_TEMP_SIMU_H_INCLUDE_GUARD
#define PA_TEMP_SIMU_H_INCLUDE_GUARD

#define PA_SIMU_TEMP_DEFAULT_RADIO_TEMP              29
#define PA_SIMU_TEMP_DEFAULT_PLATFORM_TEMP           32

#define PA_SIMU_TEMP_DEFAULT_RADIO_HIGH_WARN        110
#define PA_SIMU_TEMP_DEFAULT_RADIO_HIGH_CRIT        140

#define PA_SIMU_TEMP_DEFAULT_PLATFORM_LOW_WARN      -40
#define PA_SIMU_TEMP_DEFAULT_PLATFORM_HIGH_WARN      85
#define PA_SIMU_TEMP_DEFAULT_PLATFORM_LOW_CRIT      -45
#define PA_SIMU_TEMP_DEFAULT_PLATFORM_HIGH_CRIT     130

//--------------------------------------------------------------------------------------------------
/**
 * Set the stub return code.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_tempSimu_SetReturnCode
(
    le_result_t res
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
le_result_t pa_temp_GetPlatformThresholds
(
    int32_t* lowCriticalTempPtr,
    int32_t* lowWarningTempPtr,
    int32_t* hiWarningTempPtr,
    int32_t* hiCriticalTempPtr
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
le_result_t pa_temp_GetRadioThresholds
(
    int32_t* hiWarningTempPtr,
    int32_t* hiCriticalTempPtr
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * Trigger a temperature event report.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_tempSimu_TriggerEventReport
(
    le_temp_ThresholdStatus_t status
);

#endif // PA_TEMP_SIMU_H_INCLUDE_GUARD

