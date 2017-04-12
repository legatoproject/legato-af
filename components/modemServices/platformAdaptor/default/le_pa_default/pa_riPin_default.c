/**
 * @file pa_riPin_default.c
 *
 * Default implementation of @ref c_pa_riPin.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_riPin.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the PA Ring Indicator signal module.
 *
 * @return
 *   - LE_FAULT         The function failed.
 *   - LE_OK            The function succeeded.
 *
 * @note This function should not be called from outside the platform adapter.
 *
 * @todo Move this prototype to another (internal) header.
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
 *      - LE_OK              The function succeeded.
 *      - LE_FAULT           The function failed.
 *      - LE_BAD_PARAMETER   Bad input parameter
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_riPin_AmIOwnerOfRingSignal
(
    bool* amIOwnerPtr ///< [OUT] true when application core is the owner of the Ring Indicator
                      ///        signal,
                      ///        false when modem core is the owner of the Ring Indicator signal
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Take control of the Ring Indicator signal.
 *
 * @return
 *      - LE_OK           The function succeeded.
 *      - LE_FAULT        The function failed.
 *      - LE_UNSUPPORTED  The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_riPin_TakeRingSignal
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release control of the Ring Indicator signal.
 *
 * @return
 *      - LE_OK           The function succeeded.
 *      - LE_FAULT        The function failed.
 *      - LE_UNSUPPORTED  The platform does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_riPin_ReleaseRingSignal
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set RI GPIO value
 */
//--------------------------------------------------------------------------------------------------
void pa_riPin_Set
(
    uint8_t     set ///< [IN] 1 to Pull up GPIO RI or 0 to lower it down
)
{
    LE_ERROR("Unsupported function called");
}
