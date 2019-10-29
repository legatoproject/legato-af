//--------------------------------------------------------------------------------------------------
/**
 * @file le_riPin.c
 *
 * This file contains the source code of the Ring Indicator signal API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pa_riPin.h"
#include "le_ms_local.h"
#include "watchdogChain.h"


//--------------------------------------------------------------------------------------------------
/**
 * Semaphore to synchronize the init function with PulseRingSignalThread().
 *
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t ThreadSemaphore;

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

    le_sem_Post(ThreadSemaphore);

    // Watchdog riPin loop
    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_MonitorEventLoop(MS_WDOG_RIPIN_LOOP, watchdogInterval);

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
    // Create a semaphore to wait the thread to be ready
    ThreadSemaphore = le_sem_Create("ThreadSem",0);

    // Init semaphore to protect duration value in le_riPin_PulseRingSignal()
    SemRef = le_sem_Create("RiPinSem", 1);

    PulseRingSignalThreadRef = le_thread_Create("PulseRingSignalThread",
                                                PulseRingSignalThread,
                                                NULL);
    le_thread_Start(PulseRingSignalThreadRef);

    le_sem_Wait(ThreadSemaphore);

    return LE_OK;
}

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
le_result_t le_riPin_AmIOwnerOfRingSignal
(
    bool* amIOwnerPtr ///< [OUT] true when application core is the owner of the Ring Indicator
                      ///        signal,
                      ///        false when modem core is the owner of the Ring Indicator signal
)
{
    return pa_riPin_AmIOwnerOfRingSignal(amIOwnerPtr);
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
le_result_t le_riPin_TakeRingSignal
(
    void
)
{
    return pa_riPin_TakeRingSignal();
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
le_result_t le_riPin_ReleaseRingSignal
(
    void
)
{
    return pa_riPin_ReleaseRingSignal();
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
    le_result_t res;

    res = pa_riPin_AmIOwnerOfRingSignal(&isAppCoreOwner);

    if (LE_OK != res)
    {
        LE_ERROR("Cannot determine the RI pin owner");
        return;
    }

    if (false == isAppCoreOwner)
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
