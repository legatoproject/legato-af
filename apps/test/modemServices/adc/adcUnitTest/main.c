/**
 * This module implements the unit tests for ADC API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_adc_simu.h"
#include "le_adc_interface.h"


//--------------------------------------------------------------------------------------------------
/**
 * Test: le_adc_ReadValue()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_adc_ReadValue
(
    void
)
{
    //Dedicated string for le_adc_ReadValue()
    const char adcName[] = "EXT_ADC1";
    int32_t adcValuePtr = 0;

    // Set Return Code
    pa_adcSimu_SetReturnCode(LE_FAULT);
    LE_ASSERT(le_adc_ReadValue(adcName, &adcValuePtr) == LE_FAULT);

    // Set Return Code
    pa_adcSimu_SetReturnCode(LE_OK);
    LE_ASSERT(le_adc_ReadValue(adcName, &adcValuePtr) == LE_OK);
    LE_INFO("ADC value obtained = %d ", adcValuePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // To reactivate for all DEBUG logs
    //le_log_SetFilterLevel(LE_LOG_DEBUG);

    // Init pa simu
    pa_adc_Init();
    LE_INFO("======== UnitTest of ADC API Started ========");

    LE_INFO("========  Testle_adc_ReadValue Test ========");
    Testle_adc_ReadValue();

    LE_INFO("======== UnitTest of ADC API FINISHED ========");
    exit(0);
}
