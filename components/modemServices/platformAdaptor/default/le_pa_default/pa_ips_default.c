/**
 * @file pa_ips_default.c
 *
 * Default implementation of @ref c_pa_ips.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_ips.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the PA Input Power Supply module.
 *
 * @return
 *   - LE_FAULT         The function failed to initialize the PA Input Power Supply module.
 *   - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ips_Init
(
    void
)
{
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
    LE_ERROR("Unsupported function called");
    return NULL;
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Platform warning and critical input voltage thresholds in [mV].
 *  When thresholds input voltage are reached, a input voltage event is triggered.
 *
 * @return
 *      - LE_OK            The function succeeded.
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform power source.
 *
 * @return
 *      - LE_OK             The function succeeded.
 *      - LE_BAD_PARAMETER  Null pointer provided as a parameter.
 *      - LE_FAULT          The function failed to get the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ips_GetPowerSource
(
    le_ips_PowerSource_t* powerSourcePtr    ///< [OUT] The power source.
)
{
    if (!powerSourcePtr)
    {
        LE_ERROR("powerSourcePtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform battery level in percent.
 *
 * @return
 *      - LE_OK             The function succeeded.
 *      - LE_BAD_PARAMETER  Null pointer provided as a parameter.
 *      - LE_FAULT          The function failed to get the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_ips_GetBatteryLevel
(
    uint8_t* batteryLevelPtr    ///< [OUT] The battery level in percent.
)
{
    if (!batteryLevelPtr)
    {
        LE_ERROR("batteryLevelPtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}
