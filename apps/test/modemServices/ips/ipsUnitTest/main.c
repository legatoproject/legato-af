/**
 * This module implements the unit tests for IPS API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */


#include "legato.h"
#include "interfaces.h"
#include "le_ips_local.h"
#include "log.h"
#include "pa_ips.h"
#include "pa_ips_simu.h"

#define TEST_IPS_HI_CRITICAL_THRESHOLD    4000
#define TEST_IPS_NORMAL_THRESHOLD         3700
#define TEST_IPS_WARNING_THRESHOLD        3600
#define TEST_IPS_CRITICAL_THRESHOLD       3400

//--------------------------------------------------------------------------------------------------
// Begin Stubbed functions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Input voltage Threshold events function handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void ThresholdEventHandlerFunc
(
    le_ips_ThresholdStatus_t event,
    void* contextPtr
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_AddThresholdEventHandler(), le_ips_RemoveThresholdEventHandler()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_AddRemoveThresholdEventHandler
(
    void
)
{
    le_ips_ThresholdEventHandlerRef_t handlerRef;

    LE_INFO("======== Testle_ips_AddRemoveThresholdEventHandler Test ========");
    handlerRef = le_ips_AddThresholdEventHandler(ThresholdEventHandlerFunc, NULL);
    LE_ASSERT(handlerRef != NULL);
    le_ips_RemoveThresholdEventHandler(handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_GetInputVoltage()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_GetInputVoltage
(
    void
)
{
    uint32_t voltage = 0;
    LE_INFO("========  Testle_ips_GetInputVoltage Test ========");
    LE_ASSERT(le_ips_GetInputVoltage(&voltage) == LE_OK);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_GetVoltageThresholds(), le_ips_SetVoltageThresholds()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_SetGetVoltageThresholds
(
    void
)
{
    uint16_t criticalInVoltOri, warningInVoltOri, normalInVoltOri, hiCriticalInVoltOri;
    uint16_t criticalInVolt, warningInVolt, normalInVolt, hiCriticalInVolt;

    LE_INFO("======== Testle_ips_SetGetVoltageThresholds Test ========");
    LE_ASSERT(le_ips_GetVoltageThresholds(
        &criticalInVoltOri,
        &warningInVoltOri,
        &normalInVoltOri,
        &hiCriticalInVoltOri) == LE_OK);

    LE_ASSERT(le_ips_SetVoltageThresholds(
        TEST_IPS_CRITICAL_THRESHOLD,
        TEST_IPS_CRITICAL_THRESHOLD,
        TEST_IPS_NORMAL_THRESHOLD,
        TEST_IPS_HI_CRITICAL_THRESHOLD) == LE_BAD_PARAMETER);

    LE_ASSERT(le_ips_SetVoltageThresholds(
        TEST_IPS_CRITICAL_THRESHOLD,
        TEST_IPS_NORMAL_THRESHOLD,
        TEST_IPS_NORMAL_THRESHOLD,
        TEST_IPS_HI_CRITICAL_THRESHOLD) == LE_BAD_PARAMETER);

    LE_ASSERT(le_ips_SetVoltageThresholds(
        TEST_IPS_CRITICAL_THRESHOLD,
        TEST_IPS_NORMAL_THRESHOLD,
        TEST_IPS_WARNING_THRESHOLD,
        TEST_IPS_HI_CRITICAL_THRESHOLD) == LE_BAD_PARAMETER);

    LE_ASSERT(le_ips_SetVoltageThresholds(
        TEST_IPS_CRITICAL_THRESHOLD,
        TEST_IPS_WARNING_THRESHOLD,
        TEST_IPS_WARNING_THRESHOLD,
        TEST_IPS_HI_CRITICAL_THRESHOLD) == LE_BAD_PARAMETER);

    LE_ASSERT(le_ips_SetVoltageThresholds(
        TEST_IPS_CRITICAL_THRESHOLD,
        TEST_IPS_WARNING_THRESHOLD,
        TEST_IPS_NORMAL_THRESHOLD,
        TEST_IPS_NORMAL_THRESHOLD) == LE_BAD_PARAMETER);

    LE_ASSERT(le_ips_SetVoltageThresholds(
        TEST_IPS_CRITICAL_THRESHOLD,
        TEST_IPS_WARNING_THRESHOLD,
        TEST_IPS_HI_CRITICAL_THRESHOLD-1,
        TEST_IPS_HI_CRITICAL_THRESHOLD) == LE_BAD_PARAMETER);

    LE_ASSERT(le_ips_SetVoltageThresholds(
        TEST_IPS_CRITICAL_THRESHOLD,
        TEST_IPS_WARNING_THRESHOLD,
        TEST_IPS_HI_CRITICAL_THRESHOLD-2,
        TEST_IPS_HI_CRITICAL_THRESHOLD) == LE_OK);

    LE_ASSERT( le_ips_SetVoltageThresholds(
        TEST_IPS_CRITICAL_THRESHOLD,
        TEST_IPS_WARNING_THRESHOLD,
        TEST_IPS_NORMAL_THRESHOLD,
        TEST_IPS_HI_CRITICAL_THRESHOLD) == LE_OK);

    LE_ASSERT( le_ips_GetVoltageThresholds( &criticalInVolt, &warningInVolt,
            &normalInVolt, &hiCriticalInVolt) == LE_OK);

    LE_ASSERT( criticalInVolt == TEST_IPS_CRITICAL_THRESHOLD );
    LE_ASSERT( warningInVolt == TEST_IPS_WARNING_THRESHOLD );
    LE_ASSERT( normalInVolt == TEST_IPS_NORMAL_THRESHOLD );
    LE_ASSERT( hiCriticalInVolt == TEST_IPS_HI_CRITICAL_THRESHOLD );

    LE_ASSERT(le_ips_SetVoltageThresholds(
        criticalInVoltOri,
        warningInVoltOri,
        normalInVoltOri,
        hiCriticalInVoltOri) == LE_OK);
}


//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    pa_ipsSimu_Init();
    le_ips_Init();

    LE_INFO("======== Start UnitTest of IPS API ========");

    Testle_ips_GetInputVoltage();
    Testle_ips_AddRemoveThresholdEventHandler();
    Testle_ips_SetGetVoltageThresholds();

    LE_INFO("======== UnitTest of IPS API ends with SUCCESS ========");

    exit(0);
}
