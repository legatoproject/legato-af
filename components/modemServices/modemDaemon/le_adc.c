//--------------------------------------------------------------------------------------------------
/**
 * @file le_adc.c
 *
 * This file contains the source code of the high level ADC API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pa_adc.h"

//--------------------------------------------------------------------------------------------------
/**
 * Read values from the ADC channels
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_adc_ReadValue
(
    le_adc_AdcChannelInput_t  adcChannel, ///< [IN] channel to sample
    int32_t*                  adcValuePtr ///< [OUT] sample value

)
{
    if (adcValuePtr == NULL)
    {
        LE_KILL_CLIENT("Parameters pointer are NULL!!");
        return LE_FAULT;
    }
    return pa_adc_ReadValue(adcChannel, adcValuePtr);
}

