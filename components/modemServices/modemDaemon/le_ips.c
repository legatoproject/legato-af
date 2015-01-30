//--------------------------------------------------------------------------------------------------
/**
 * @file le_ips.c
 *
 * This file contains the source code of the Input Power Supply Monitoring API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

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
le_result_t le_ips_GetInputVoltage
(
    uint32_t* inputVoltagePtr
        ///< [OUT]
        ///< [OUT] The input voltage in [mV]
)
{
    if (inputVoltagePtr == NULL)
    {
        LE_KILL_CLIENT("inputVoltagePtr is NULL!");
        return LE_FAULT;
    }

   return  pa_ips_GetInputVoltage(inputVoltagePtr);
}
