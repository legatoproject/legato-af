/**
 * @file pa_riPin_simu.c
 *
 * Simulation implementation of PA Ring Indicator signal.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "pa_riPin.h"

static le_result_t ReturnCode = LE_FAULT;
static bool AmIOwner = false;
uint32_t PulseRingSignalDuration = 0;

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Set the stub return code.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_riPinSimu_SetReturnCode
(
    le_result_t res
)
{
    ReturnCode = res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the "AmIOwner" flag
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_riPinSimu_SetAmIOwnerOfRingSignal
(
    bool amIOwner
)
{
    AmIOwner = amIOwner;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the "AmIOwner" value
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_riPinSimu_CheckAmIOwnerOfRingSignal
(
    bool amIOwner
)
{
    LE_ASSERT(AmIOwner == amIOwner);
}

//--------------------------------------------------------------------------------------------------
/**
 * Check duration value
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_riPinSimu_CheckPulseRingSignalDuration
(
    uint32_t duration
)
{
    LE_ASSERT(PulseRingSignalDuration == duration);
}

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
    if (ReturnCode == LE_OK)
    {
        *amIOwnerPtr = AmIOwner;
    }

    return ReturnCode;
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
    if(ReturnCode == LE_OK)
    {
        AmIOwner = true;
    }

    return ReturnCode;
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
    if(ReturnCode == LE_OK)
    {
        AmIOwner = false;
    }

    return ReturnCode;
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
    PulseRingSignalDuration = duration;

    return ;
}

