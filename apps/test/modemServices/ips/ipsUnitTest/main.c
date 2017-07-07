/**
 * This module implements the unit tests for IPS API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "le_ips_local.h"
#include "log.h"
#include "pa_ips.h"
#include "pa_ips_simu.h"

//--------------------------------------------------------------------------------------------------
/**
 * Voltage thresholds for tests
 */
//--------------------------------------------------------------------------------------------------
#define TEST_IPS_HI_CRITICAL_THRESHOLD      4000
#define TEST_IPS_NORMAL_THRESHOLD           3700
#define TEST_IPS_WARNING_THRESHOLD          3600
#define TEST_IPS_CRITICAL_THRESHOLD         3400

//--------------------------------------------------------------------------------------------------
/**
 * Voltage for tests
 */
//--------------------------------------------------------------------------------------------------
#define TEST_IPS_VOLTAGE                    3900

//--------------------------------------------------------------------------------------------------
/**
 * Battery level for tests
 */
//--------------------------------------------------------------------------------------------------
#define TEST_IPS_BATTERY_LEVEL              57

//--------------------------------------------------------------------------------------------------
/**
 * External battery level for tests
 */
//--------------------------------------------------------------------------------------------------
#define TEST_IPS_EXT_BATTERY_LEVEL          100


//--------------------------------------------------------------------------------------------------
/**
 * Input voltage Threshold events function handler
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
    uint32_t voltage;
    LE_INFO("========  Testle_ips_GetInputVoltage Test ========");
    pa_ipsSimu_SetInputVoltage(TEST_IPS_VOLTAGE);
    LE_ASSERT_OK(le_ips_GetInputVoltage(&voltage));
    LE_ASSERT(TEST_IPS_VOLTAGE == voltage);
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
    LE_ASSERT_OK(le_ips_GetVoltageThresholds(&criticalInVoltOri,
                                             &warningInVoltOri,
                                             &normalInVoltOri,
                                             &hiCriticalInVoltOri));

    LE_ASSERT(LE_BAD_PARAMETER == le_ips_SetVoltageThresholds(TEST_IPS_CRITICAL_THRESHOLD,
                                                              TEST_IPS_CRITICAL_THRESHOLD,
                                                              TEST_IPS_NORMAL_THRESHOLD,
                                                              TEST_IPS_HI_CRITICAL_THRESHOLD));

    LE_ASSERT(LE_BAD_PARAMETER == le_ips_SetVoltageThresholds(TEST_IPS_CRITICAL_THRESHOLD,
                                                              TEST_IPS_NORMAL_THRESHOLD,
                                                              TEST_IPS_NORMAL_THRESHOLD,
                                                              TEST_IPS_HI_CRITICAL_THRESHOLD));

    LE_ASSERT(LE_BAD_PARAMETER == le_ips_SetVoltageThresholds(TEST_IPS_CRITICAL_THRESHOLD,
                                                              TEST_IPS_NORMAL_THRESHOLD,
                                                              TEST_IPS_WARNING_THRESHOLD,
                                                              TEST_IPS_HI_CRITICAL_THRESHOLD));

    LE_ASSERT(LE_BAD_PARAMETER == le_ips_SetVoltageThresholds(TEST_IPS_CRITICAL_THRESHOLD,
                                                              TEST_IPS_WARNING_THRESHOLD,
                                                              TEST_IPS_WARNING_THRESHOLD,
                                                              TEST_IPS_HI_CRITICAL_THRESHOLD));

    LE_ASSERT(LE_BAD_PARAMETER == le_ips_SetVoltageThresholds(TEST_IPS_CRITICAL_THRESHOLD,
                                                              TEST_IPS_WARNING_THRESHOLD,
                                                              TEST_IPS_NORMAL_THRESHOLD,
                                                              TEST_IPS_NORMAL_THRESHOLD));

    LE_ASSERT(LE_BAD_PARAMETER == le_ips_SetVoltageThresholds(TEST_IPS_CRITICAL_THRESHOLD,
                                                              TEST_IPS_WARNING_THRESHOLD,
                                                              TEST_IPS_HI_CRITICAL_THRESHOLD-1,
                                                              TEST_IPS_HI_CRITICAL_THRESHOLD));

    LE_ASSERT_OK(le_ips_SetVoltageThresholds(TEST_IPS_CRITICAL_THRESHOLD,
                                             TEST_IPS_WARNING_THRESHOLD,
                                             TEST_IPS_HI_CRITICAL_THRESHOLD-2,
                                             TEST_IPS_HI_CRITICAL_THRESHOLD));

    LE_ASSERT_OK(le_ips_SetVoltageThresholds(TEST_IPS_CRITICAL_THRESHOLD,
                                             TEST_IPS_WARNING_THRESHOLD,
                                             TEST_IPS_NORMAL_THRESHOLD,
                                             TEST_IPS_HI_CRITICAL_THRESHOLD));

    LE_ASSERT_OK(le_ips_GetVoltageThresholds(&criticalInVolt,
                                             &warningInVolt,
                                             &normalInVolt,
                                             &hiCriticalInVolt));

    LE_ASSERT(criticalInVolt == TEST_IPS_CRITICAL_THRESHOLD);
    LE_ASSERT(warningInVolt == TEST_IPS_WARNING_THRESHOLD);
    LE_ASSERT(normalInVolt == TEST_IPS_NORMAL_THRESHOLD);
    LE_ASSERT(hiCriticalInVolt == TEST_IPS_HI_CRITICAL_THRESHOLD);

    LE_ASSERT_OK(le_ips_SetVoltageThresholds(criticalInVoltOri,
                                             warningInVoltOri,
                                             normalInVoltOri,
                                             hiCriticalInVoltOri));
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_GetPowerSource()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_GetPowerSource
(
    void
)
{
    le_ips_PowerSource_t powerSource;
    LE_INFO("======== Testle_ips_GetPowerSource Test ========");
    pa_ipsSimu_SetPowerSource(LE_IPS_POWER_SOURCE_EXTERNAL);
    LE_ASSERT_OK(le_ips_GetPowerSource(&powerSource));
    LE_ASSERT(LE_IPS_POWER_SOURCE_EXTERNAL == powerSource);
    pa_ipsSimu_SetPowerSource(LE_IPS_POWER_SOURCE_BATTERY);
    LE_ASSERT_OK(le_ips_GetPowerSource(&powerSource));
    LE_ASSERT(LE_IPS_POWER_SOURCE_BATTERY == powerSource);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_GetBatteryLevel()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_GetBatteryLevel
(
    void
)
{
    uint8_t batteryLevel;
    LE_INFO("======== Testle_ips_GetBatteryLevel Test ========");
    pa_ipsSimu_SetBatteryLevel(TEST_IPS_BATTERY_LEVEL);
    LE_ASSERT_OK(le_ips_GetBatteryLevel(&batteryLevel));
    LE_ASSERT(TEST_IPS_BATTERY_LEVEL == batteryLevel);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_SetBatteryLevel()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_SetBatteryLevel
(
    void
)
{
    le_ips_PowerSource_t powerSource;
    uint8_t batteryLevel;
    LE_INFO("======== Testle_ips_SetBatteryLevel Test ========");
    // Set different PA values in order to check that these ones are not used
    pa_ipsSimu_SetBatteryLevel(TEST_IPS_BATTERY_LEVEL);
    pa_ipsSimu_SetPowerSource(LE_IPS_POWER_SOURCE_EXTERNAL);
    LE_ASSERT(LE_BAD_PARAMETER == le_ips_SetBatteryLevel(TEST_IPS_EXT_BATTERY_LEVEL + 1));
    LE_ASSERT_OK(le_ips_SetBatteryLevel(TEST_IPS_EXT_BATTERY_LEVEL));
    LE_ASSERT_OK(le_ips_GetBatteryLevel(&batteryLevel));
    LE_ASSERT(TEST_IPS_EXT_BATTERY_LEVEL == batteryLevel);
    LE_ASSERT_OK(le_ips_GetPowerSource(&powerSource));
    LE_ASSERT(LE_IPS_POWER_SOURCE_BATTERY == powerSource);
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
    Testle_ips_GetPowerSource();
    Testle_ips_GetBatteryLevel();
    Testle_ips_SetBatteryLevel();

    LE_INFO("======== UnitTest of IPS API ends with SUCCESS ========");

    exit(EXIT_SUCCESS);
}
