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
/**
 * Semaphore to protect duration value in le_riPin_PulseRingSignal().
 *
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t SemRef;

//--------------------------------------------------------------------------------------------------
/**
 * Thread reference used by Pulse function.
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t  PulseRingSignalThreadRef;

//--------------------------------------------------------------------------------------------------
/**
 * Timer reference used by Pulse function.
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t  RiDurationTimerRef;

//--------------------------------------------------------------------------------------------------
/**
 * Pulse duration in ms.
 */
//--------------------------------------------------------------------------------------------------
static size_t PulseDuration;

//--------------------------------------------------------------------------------------------------
/**
 * RI duration Timer handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void RiDurationTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    // Lower GPIO RI
    pa_riPin_Set(0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Start to pulse the signal
 *
 */
//--------------------------------------------------------------------------------------------------
static void PulseSignal
(
    void* param1Ptr, // not used
    void* param2Ptr  // not used
)
{
    if (le_timer_IsRunning(RiDurationTimerRef))
    {
        LE_WARN("The signal is already pulsed!");
    }
    else
    {
        if (le_timer_SetMsInterval(RiDurationTimerRef, PulseDuration) != LE_OK)
        {
            LE_WARN("Cannot set Interval timer!");
        }
        else
        {
            // Pull up GPIO RI
            pa_riPin_Set(1);

            le_timer_Start(RiDurationTimerRef);
        }
    }
    le_sem_Post(SemRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This thread creates the RiDuration timer.
 */
//--------------------------------------------------------------------------------------------------
static void* PulseRingSignalThread
(
    void* contextPtr
)
{
    if ((RiDurationTimerRef = le_timer_Create("RiDurationTimer")) == NULL)
    {
        LE_ERROR("Could not create the RiDurationTimer");
        return NULL;
    }
    if (le_timer_SetHandler(RiDurationTimerRef, RiDurationTimerHandler) != LE_OK)
    {
        LE_ERROR("Could not set the handler for RiDurationTimer");
        return NULL;
    }

    // Run the event loop
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the riPin service
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_riPin_Init
(
    void
)
{
    // Init semaphore to protect duration value in le_riPin_PulseRingSignal()
    SemRef = le_sem_Create("RiPinSem", 1);

    PulseRingSignalThreadRef = le_thread_Create("PulseRingSignalThread",
                                                PulseRingSignalThread,
                                                NULL);
    le_thread_Start(PulseRingSignalThreadRef);

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
le_result_t le_riPin_AmIOwnerOfRingSignal
(
    bool* amIOwnerPtr ///< true when application core is the owner of the Ring Indicator signal,
                      ///  false when modem core is the owner of the Ring Indicator signal.
)
{
    if(pa_riPin_AmIOwnerOfRingSignal(amIOwnerPtr) == LE_OK)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
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
le_result_t le_riPin_TakeRingSignal
(
    void
)
{
    if(pa_riPin_TakeRingSignal() == LE_OK)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
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
    if(pa_riPin_ReleaseRingSignal() == LE_OK)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
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
    bool isAppCoreOwner;

    if (pa_riPin_AmIOwnerOfRingSignal(&isAppCoreOwner) != LE_OK)
    {
        LE_ERROR("Cannot determine the RI pin owner");
        return;
    }

    if (!isAppCoreOwner)
    {
        LE_WARN("Cannot perform this operation, Modem core is the owner of the signal!");
        return;
    }
    else
    {
        le_sem_Wait(SemRef);
        PulseDuration = duration;

        le_event_QueueFunctionToThread(PulseRingSignalThreadRef,
                                       PulseSignal,
                                       NULL,
                                       NULL);
    }
}
