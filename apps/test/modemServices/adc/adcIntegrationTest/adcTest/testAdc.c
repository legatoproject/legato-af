/**
 * This module implements the component tests for ADC API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Number of times to read the ADC
 */
//--------------------------------------------------------------------------------------------------
#define ADC_TEST_ITERATIONS 5


COMPONENT_INIT
{
    //Dedicated string for le_adc_ReadValue()
    const char actualAdcName[] = "EXT_ADC1";
    const char outofrangeAdcName[] = "EXT_ADC9";
    int32_t adcValue = 0;
    int i;

    LE_TEST_PLAN(ADC_TEST_ITERATIONS + 1);

    for (i = 0; i < ADC_TEST_ITERATIONS; ++i)
    {
        LE_TEST_OK(le_adc_ReadValue(actualAdcName, &adcValue) == LE_OK,
                   "Read adc %s (iteration %d)", actualAdcName, i);
        LE_TEST_INFO("ADC result %d: %"PRIu32, i, adcValue);
    }

    LE_TEST_OK(le_adc_ReadValue(outofrangeAdcName, &adcValue) == LE_FAULT,
               "Read invalid adc %s", outofrangeAdcName);
    LE_TEST_EXIT;
}
