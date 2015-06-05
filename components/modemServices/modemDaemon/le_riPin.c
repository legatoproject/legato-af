//--------------------------------------------------------------------------------------------------
/**
 * @file le_riPin.c
 *
 * This file contains the source code of the Ring Indicator signal API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pa_riPin.h"


//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Take control of the Ring Indicator signal.
 *
 * @return
 *      - LE_OK     The function succeeded.
 *      - LE_FAULT  The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_riPin_TakeRingSignal
(
    void
)
{
    return  pa_riPin_TakeRingSignal();
}

//--------------------------------------------------------------------------------------------------
/**
 * Release control of the Ring Indicator signal.
 *
 * @return
 *      - LE_OK     The function succeeded.
 *      - LE_FAULT  The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_riPin_ReleaseRingSignal
(
    void
)
{
    return  pa_riPin_ReleaseRingSignal();
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the Ring Indicator signal high for configurable duration of time before lowering it.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_riPin_PulseRingSignal
(
    uint32_t duration ///< [IN] duration in ms
)
{
    pa_riPin_PulseRingSignal(duration);
}
