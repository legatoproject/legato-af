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

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform voltage input in [mV].
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
