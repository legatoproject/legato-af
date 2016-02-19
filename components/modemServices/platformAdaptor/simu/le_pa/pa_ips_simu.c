/**
 * @file pa_ips_simu.c
 *
 * SIMU implementation of pa Input Power Supply API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_ips.h"
#include "pa_ips_simu.h"

//--------------------------------------------------------------------------------------------------
/**
 * Default AR/WP Input Voltage thresholds
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_IPS_HICRITICAL_THRESHOLD   4400
#define DEFAULT_IPS_NORMAL_THRESHOLD       3600
#define DEFAULT_ISP_WARNING_THRESHOLD      3400
#define DEFAULT_IPS_CRITICAL_THRESHOLD     3200

//--------------------------------------------------------------------------------------------------
/**
 * Input Voltage status threshold Event Id
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t VoltageThresholdEventId;

//--------------------------------------------------------------------------------------------------
/**
 * The internal input Voltage.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t InputVoltage = 3900;

//--------------------------------------------------------------------------------------------------
/**
 * The internal input Voltage thresholds.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t HiCriticalVoltThreshold = DEFAULT_IPS_HICRITICAL_THRESHOLD;
static uint32_t NormalVoltThershold = DEFAULT_IPS_NORMAL_THRESHOLD;
static uint32_t WarningVoltThreshold = DEFAULT_ISP_WARNING_THRESHOLD;
static uint32_t CriticalVoltThreshold = DEFAULT_IPS_CRITICAL_THRESHOLD;


//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function set the input voltage simulation value in [mV].
 */
//--------------------------------------------------------------------------------------------------
void pa_ipsSimu_SetInputVoltage
(
    uint32_t inputVoltage
        ///< [IN]
        ///< [IN] The input voltage in [mV]
)
{
    InputVoltage = inputVoltage;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform input voltage in [mV].
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ips_GetInputVoltage
(
    uint32_t* inputVoltagePtr
        ///< [OUT]
        ///< [OUT] The input voltage in [mV]
)
{
    *inputVoltagePtr = InputVoltage;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to add an input voltage status notification handler
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t* pa_ips_AddVoltageEventHandler
(
    pa_ips_ThresholdInd_HandlerFunc_t   msgHandler
)
{
    le_event_HandlerRef_t  handlerRef = NULL;

    if ( msgHandler != NULL )
    {
        handlerRef = le_event_AddHandler( "VoltThresholdStatushandler",
            VoltageThresholdEventId,
            (le_event_HandlerFunc_t) msgHandler);
    }
    else
    {
        LE_ERROR("Null handler given in parameter");
    }

    return (le_event_HandlerRef_t*) handlerRef;
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the Platform warning and critical input voltage thresholds in [mV].
 *  When thresholds input voltage are reached, a input voltage event is triggered.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_OUT_OF_RANGE  One or more thresholds are out of the range allowed by the platform.
 *      - LE_FAULT         The function failed to set the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_SetVoltageThresholds
(
    uint16_t criticalVolt,
        ///< [IN]
        ///< [IN] The critical input voltage threshold in [mV].

    uint16_t warningVolt,
        ///< [IN]
        ///< [IN] The warning input voltage threshold in [mV].

    uint16_t normalVolt,
        ///< [IN]
        ///< [IN] The normal input voltage threshold in [mV].

    uint16_t hiCriticalVolt
        ///< [IN]
        ///< [IN] The high critical input voltage threshold in [mV].
)
{
    NormalVoltThershold = normalVolt;
    WarningVoltThreshold = warningVolt;
    CriticalVoltThreshold = criticalVolt;
    HiCriticalVoltThreshold = hiCriticalVolt;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform warning and critical input voltage thresholds in [mV].
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the thresholds.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_GetVoltageThresholds
(
    uint16_t* criticalVoltPtr,
        ///< [OUT]
        ///< [OUT] The critical input voltage threshold in [mV].

    uint16_t* warningVoltPtr,
        ///< [OUT]
        ///< [OUT] The warning input voltage threshold in [mV].

    uint16_t* normalVoltPtr,
        ///< [OUT]
        ///< [OUT] The normal input voltage threshold in [mV].

    uint16_t* hiCriticalVoltPtr
        ///< [OUT]
        ///< [IN] The high critical input voltage threshold in [mV].
)
{
    *normalVoltPtr = NormalVoltThershold;
    *warningVoltPtr = WarningVoltThreshold;
    *criticalVoltPtr = CriticalVoltThreshold;
    *hiCriticalVoltPtr = HiCriticalVoltThreshold;

    return LE_OK;
}



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
)
{
    // Create the event for signaling user handlers.
    VoltageThresholdEventId = le_event_CreateIdWithRefCounting("VoltageStatusEvent");

    LE_INFO("simulation pa_ips init - stub");

    return LE_OK;
}
