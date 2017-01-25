/**
 * @file pa_adc_default.c
 *
 * Default implementation of @ref c_pa_adc.
 *
 * Copyright (C) Sierra Wireless Inc.
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
le_result_t pa_adc_ReadValue
(
    const char* adcNamePtr,
        ///< [IN]
        ///< Name of the ADC to read.

    int32_t* adcValuePtr
        ///< [OUT]
        ///< The adc value
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}
