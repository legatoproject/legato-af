/** @file pa_riPin.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PARIPIN_INCLUDE_GUARD
#define LEGATO_PARIPIN_INCLUDE_GUARD

#include "legato.h"
#include "interfaces.h"

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
);

//--------------------------------------------------------------------------------------------------
/**
 * Check whether the application core is the current owner of the Ring Indicator signal.
 *
 * @return
 *      - LE_OK              The function succeeded.
 *      - LE_FAULT           The function failed.
 *      - LE_BAD_PARAMETER   Bad input parameter.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_riPin_AmIOwnerOfRingSignal
(
    bool* amIOwnerPtr ///< [OUT] true when application core is the owner of the Ring Indicator
                      ///        signal,
                      ///        false when modem core is the owner of the Ring Indicator signal.
);

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
LE_SHARED le_result_t pa_riPin_TakeRingSignal
(
    void
);

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
LE_SHARED le_result_t pa_riPin_ReleaseRingSignal
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Set RI GPIO value
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_riPin_Set
(
    uint8_t     set ///< [IN] 1 to Pull up GPIO RI or 0 to lower it down
);

#endif // LEGATO_PARIPIN_INCLUDE_GUARD
