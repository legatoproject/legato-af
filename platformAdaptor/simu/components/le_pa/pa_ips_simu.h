/**
 * @file pa_ips_simu.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#ifndef LEGATO_PA_IPS_SIMU_INCLUDE_GUARD
#define LEGATO_PA_IPS_SIMU_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * This function set the input voltage simulation value in [mV].
 */
//--------------------------------------------------------------------------------------------------
void pa_ipsSimu_SetInputVoltage
(
    uint32_t inputVoltagePtr
        ///< [IN]
        ///< [IN] The input voltage in [mV]
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the PA Input Power Supply module.
 *
 * @return
 *   - LE_FAULT         The function failed to initialize the PA Input Power Supply module.
 *   - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ipsSimu_Init
(
    void
);

#endif
