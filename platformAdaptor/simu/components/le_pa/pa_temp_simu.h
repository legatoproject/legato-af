/** @file pa_temp_simu.h
 *
 * Legato @ref pa_temp_simu include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef PA_TEMP_SIMU_H_INCLUDE_GUARD
#define PA_TEMP_SIMU_H_INCLUDE_GUARD


#define PA_SIMU_TEMP_DEFAULT_TEMPERATURE    29
#define PA_SIMU_TEMP_SENSOR                 "SIMU_TEMP_SENSOR"
#define PA_SIMU_TEMP_DEFAULT_HI_CRIT        50

//--------------------------------------------------------------------------------------------------
/**
 * Set the stubbed return code.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_tempSimu_SetReturnCode
(
    le_result_t res
);

//--------------------------------------------------------------------------------------------------
/**
 * Trigger a temperature event report.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_tempSimu_TriggerEventReport
(
    const char*  thresholdPtr  ///< [IN] Name of the threshold.
);

#endif // PA_TEMP_SIMU_H_INCLUDE_GUARD

