/** @file paQmiVoiceTest.c
 *
 * This voice call test runs in two modes: making a call and receiving a call.
 *
 * If this program is run with no arguments then this test program will wait for an incoming call.
 * Once an incoming call is received the call will be answered and held for 60 seconds then this
 * program will hangup.
 *
 * If this program is run with an argument then this test program will make a call and assume the
 * argument is the number to call.  This test will still answer incoming calls if there is no
 * ongoing calls.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


#include "legato.h"
#include "pa.h"
#include "pa_mcc.h"


static le_timer_Ref_t HangUpTimer;

static void CallEventHandler
(
    pa_mcc_CallEventData_t*  dataPtr  ///< Data received with the handler
)
{
    LE_INFO("Call Event type: %d", dataPtr->event);
    LE_INFO("Call Event number: %s", dataPtr->phoneNumber);
    LE_INFO("Call Event Termination reason: %d", dataPtr->TerminationEvent);

    if (dataPtr->event == LE_MCC_CALL_EVENT_INCOMING)
    {
        LE_INFO("Answering call from %s", dataPtr->phoneNumber);

        if (pa_mcc_Answer() != LE_OK)
        {
            LE_ERROR("Could not answer incoming call.");
        }
        else
        {
            LE_ASSERT(le_timer_Start(HangUpTimer) == LE_OK);
        }
    }
    else if (dataPtr->event == LE_MCC_CALL_EVENT_TERMINATED)
    {
        le_timer_Stop(HangUpTimer);
    }
}

static void HangUpTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("*************************Hanging up.");
    LE_ERROR_IF(pa_mcc_HangUp() != LE_OK, "Could not hangup.");
}


COMPONENT_INIT
{
    LE_INFO("======== Begin Voice Platform Adapter's QMI implementation Test  ========");

    LE_ASSERT(pa_Init() == LE_OK);

    LE_ASSERT(pa_mcc_SetCallEventHandler(CallEventHandler) == LE_OK);

    // The modem seems to need time to initialize.
    sleep(1);

    // Set a hang-up timer.
    HangUpTimer = le_timer_Create("HangUp");

    le_clk_Time_t interval = {60, 0};
    LE_ASSERT(le_timer_SetInterval(HangUpTimer, interval) == LE_OK);
    LE_ASSERT(le_timer_SetHandler(HangUpTimer, HangUpTimerHandler) == LE_OK);

    // Make an outgoing call if there a phone number is provided.
    char numberToCall[20];
    uint8_t callId;
    if (le_arg_GetArg(0, numberToCall, sizeof(numberToCall)) == LE_OK)
    {
        // Make an outgoing call.
        LE_INFO("Making a call to %s.", numberToCall);

        LE_ERROR_IF(pa_mcc_VoiceDial(numberToCall,
                                     PA_MCC_DEACTIVATE_CLIR,
                                     PA_MCC_DEACTIVATE_CUG,
                                     &callId) != LE_OK, "Failed to make a call." );
    }
}
