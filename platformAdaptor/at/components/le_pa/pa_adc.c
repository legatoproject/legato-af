/**
 * @file pa_adc.c
 *
 * AT implementation of ADC API - stub.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


#include "legato.h"
#include "interfaces.h"
#include "pa_adc.h"


//--------------------------------------------------------------------------------------------------
/**
 * Read the value of a given ADC channel in units appropriate to that channel.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_adc_ReadValue
(
    const char* adcNamePtr,     ///< [IN] Name of the ADC to read
    int32_t*    adcValuePtr     ///< [OUT] The adc value
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
    LE_INFO("AT pa_adc init - stub");
    return LE_OK;
}
