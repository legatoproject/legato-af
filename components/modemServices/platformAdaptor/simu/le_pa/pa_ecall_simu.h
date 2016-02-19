/** @file pa_ecall_simu.h
 *
 * Legato @ref pa_ecall_simu include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef PA_ECALL_SIMU_H_INCLUDE_GUARD
#define PA_ECALL_SIMU_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Report the eCall state
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_ecallSimu_ReportEcallState
(
    le_ecall_State_t  state
);

//--------------------------------------------------------------------------------------------------
/**
 * simu init
 *
 **/
//--------------------------------------------------------------------------------------------------
void ecall_simu_Init
(
    void
);


#endif // PA_ECALL_SIMU_H_INCLUDE_GUARD

