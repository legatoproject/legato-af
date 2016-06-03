/**
 * @file pa_ips.c
 *
 * AT implementation of pa Input Power Supply API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_ips.h"

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

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
    uint32_t* inputVoltagePtr   ///< [OUT] The input voltage in [mV]
)
{
    return LE_FAULT;
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
    return NULL;
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
    uint16_t criticalVolt,      ///< [IN] The critical input voltage threshold in [mV]
    uint16_t warningVolt,       ///< [IN] The warning input voltage threshold in [mV]
    uint16_t normalVolt,        ///< [IN] The normal input voltage threshold in [mV]
    uint16_t hiCriticalVolt     ///< [IN] The high critical input voltage threshold in [mV]
)
{
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
    uint16_t* criticalVoltPtr,      ///< [OUT] The critical input voltage threshold in [mV]
    uint16_t* warningVoltPtr,       ///< [OUT] The warning input voltage threshold in [mV]
    uint16_t* normalVoltPtr,        ///< [OUT] The normal input voltage threshold in [mV]
    uint16_t* hiCriticalVoltPtr     ///< [IN/OUT] The high critical input voltage threshold in [mV]
)
{
    return LE_FAULT;
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
le_result_t pa_ips_Init
(
    void
)
{
    return LE_OK;
}
