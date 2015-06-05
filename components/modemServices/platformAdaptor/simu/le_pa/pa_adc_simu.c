/**
 * @file pa_adc_simu.c
 *
 * simu implementation of ADC API - stub.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


#include "legato.h"
#include "interfaces.h"
#include "pa_adc.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function is a stub
 *
 * @return
 * - LE_FAULT         The function failed to get the value.
 * - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_adc_ReadValue
(
    uint8_t adcChannel,    ///< [IN] Channel to read
    int32_t* adcValuePtr   ///< [OUT] value returned by adc read
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub init function
 * 
 **/
//--------------------------------------------------------------------------------------------------

le_result_t pa_adc_Init
(
    void
)
{
    LE_INFO("simulation pa_adc init - stub");

    return LE_OK;
}
