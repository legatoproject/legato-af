/**
 * @file pa_riPin_simu.c
 *
 * AT implementation of PA Ring Indicator signal.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "pa_riPin.h"



//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the PA Ring Indicator signal module.
 *
 * @return
 *   - LE_FAULT         The function failed.
 *   - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_riPin_Init
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check whether the application core is the current owner of the Ring Indicator signal.
 *
 * @return
 *      - LE_OK     The function succeeded.
 *      - LE_FAULT  The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_riPin_AmIOwnerOfRingSignal
(
    bool* amIOwnerPtr ///< true when application core is the owner of the Ring Indicator signal,
                      ///  false when modem core is the owner of the Ring Indicator signal.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Take control of the Ring Indicator signal.
 *
 * @return
 *      - LE_OK     The function succeeded.
 *      - LE_FAULT  The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_riPin_TakeRingSignal
(
    void
)
{
    return LE_FAULT;
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
le_result_t pa_riPin_ReleaseRingSignal
(
    void
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Sets the Ring Indicator signal high for configurable duration of time before lowering it.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_riPin_PulseRingSignal
(
    uint32_t duration ///< [IN] duration in ms
)
{
    return ;
}
