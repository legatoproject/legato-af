/** @file pa_riPin_simu.h
 *
 * Legato @ref pa_riPin_simu include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef PA_RIPIN_SIMU_H_INCLUDE_GUARD
#define PA_RIPIN_SIMU_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Set the stub return code.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_riPinSimu_SetReturnCode
(
    le_result_t res
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the "AmIOwner" flag
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_riPinSimu_SetAmIOwnerOfRingSignal
(
    bool amIOwner
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the "AmIOwner" value
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_riPinSimu_CheckAmIOwnerOfRingSignal
(
    bool amIOwner
);

//--------------------------------------------------------------------------------------------------
/**
 * Check duration value
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_riPinSimu_CheckPulseRingSignalDuration
(
    uint32_t duration
);

#endif // PA_RIPIN_SIMU_H_INCLUDE_GUARD
