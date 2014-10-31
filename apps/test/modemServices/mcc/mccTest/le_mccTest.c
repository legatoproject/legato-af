 /**
  * This module implements the le_mcc's unit tests.
  *
  *
  * Copyright (C) Sierra Wireless, Inc. 2013-2014.  Use of this work is subject to license.
  *
  */

#include "legato.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <semaphore.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>

#include "interfaces.h"
#include "main.h"





#ifndef AUTOMATIC
static char  DestinationNmbr[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
#else
#define DestinationNmbr  "XXXXXXXXXXXX"
#endif

static le_mcc_call_ObjRef_t TestCallRef;
static le_timer_Ref_t HangUpTimer;



//--------------------------------------------------------------------------------------------------
/**
 * HangUp Timer handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void HangUpTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    LE_INFO("Hanging up all calls!");
    LE_ERROR_IF(le_mcc_call_HangUpAll() != LE_OK, "Could not hangup.");
}



//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Profile State Change Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyProfileStateChangeHandler(le_mcc_profile_State_t  newState, void* contextPtr)
{
    LE_INFO("MCC TEST: New profile's state: %d", newState);

    if (newState == LE_MCC_PROFILE_IDLE)
    {
        LE_INFO("Check ProfileStateChangeHandler passed, state is LE_MCC_PROFILE_IDLE.");
    }
    else if (newState == LE_MCC_PROFILE_IN_USE)
    {
        LE_INFO("Check ProfileStateChangeHandler passed, state is LE_MCC_PROFILE_IN_USE.");
    }
    else
    {
        LE_INFO("Check ProfileStateChangeHandler failed, bad new state.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Call Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyCallEventHandler(le_mcc_call_ObjRef_t  callRef, le_mcc_call_Event_t callEvent, void* contextPtr)
{
    le_result_t         res;

    LE_INFO("MCC TEST: New Call event: %d for Call %p", callEvent, callRef);

    if (callEvent == LE_MCC_CALL_EVENT_ALERTING)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_CALL_EVENT_ALERTING.");
    }
    else if (callEvent == LE_MCC_CALL_EVENT_CONNECTED)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_CALL_EVENT_CONNECTED.");
    }
    else if (callEvent == LE_MCC_CALL_EVENT_TERMINATED)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_CALL_EVENT_TERMINATED.");
        le_mcc_call_TerminationReason_t term = le_mcc_call_GetTerminationReason(callRef);
        switch(term)
        {
            case LE_MCC_CALL_TERM_NETWORK_FAIL:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_NETWORK_FAIL");
                break;

            case LE_MCC_CALL_TERM_BAD_ADDRESS:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_BAD_ADDRESS");
                break;

            case LE_MCC_CALL_TERM_BUSY:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_BUSY");
                break;

            case LE_MCC_CALL_TERM_LOCAL_ENDED:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_LOCAL_ENDED");
                break;

            case LE_MCC_CALL_TERM_REMOTE_ENDED:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_REMOTE_ENDED");
                break;

            case LE_MCC_CALL_TERM_NOT_DEFINED:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_NOT_DEFINED");
                break;

            default:
                LE_INFO("Termination reason is %d", term);
                break;
        }
        le_mcc_call_Delete(callRef);
        if (HangUpTimer)
        {
            le_timer_Stop(HangUpTimer);
        }
    }
    else if (callEvent == LE_MCC_CALL_EVENT_INCOMING)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_CALL_EVENT_INCOMING.");
        res = le_mcc_call_Answer(callRef);
        if (res == LE_OK)
        {
            LE_INFO("Check MyCallEventHandler passed, I answered the call");
            LE_INFO("All calls will be hung-up in 10 seconds");
            LE_ASSERT(le_timer_Start(HangUpTimer) == LE_OK);
        }
        else
        {
            LE_INFO("Check MyCallEventHandler failed to answer the call.");
        }
    }
    else
    {
        LE_INFO("Check MyCallEventHandler failed, unknowm event.");
    }
}



//--------------------------------------------------------------------------------------------------
//                                       Public Functions
//--------------------------------------------------------------------------------------------------

#ifndef AUTOMATIC
//--------------------------------------------------------------------------------------------------
/**
 * This function gets the telephone number from the User (interactive case).
 *
 */
//--------------------------------------------------------------------------------------------------
void GetTel(void)
{
    char *strPtr;

    do
    {
        fprintf(stderr, "Please enter the destination's telephone number to perform the MCC tests: \n");
        strPtr=fgets ((char*)DestinationNmbr, 16, stdin);
    }while (strlen(strPtr) == 0);

    DestinationNmbr[strlen(DestinationNmbr)-1]='\0';
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Test: Modem Profile.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_mcc_Profile()
{
    le_mcc_profile_ObjRef_t                  profileRef;
    le_mcc_profile_State_t                   profileState;
    le_mcc_profile_StateChangeHandlerRef_t   profileStateChangeHandlerRef;
    le_mcc_profile_CallEventHandlerRef_t     profileCallEventHandlerRef;

    profileRef=le_mcc_profile_GetByName("Modem-Sim1");
    CU_ASSERT_PTR_NOT_NULL(profileRef);

    profileState=le_mcc_profile_GetState(profileRef);
    CU_ASSERT_TRUE((profileState>=LE_MCC_PROFILE_NOT_AVAILABLE) && (profileState<=LE_MCC_PROFILE_IN_USE));

    profileStateChangeHandlerRef = le_mcc_profile_AddStateChangeHandler(profileRef,
                                                                        MyProfileStateChangeHandler,
                                                                        NULL);
    CU_ASSERT_PTR_NOT_NULL(profileStateChangeHandlerRef);

    profileCallEventHandlerRef = le_mcc_profile_AddCallEventHandler(profileRef, MyCallEventHandler, NULL);

    le_mcc_profile_RemoveStateChangeHandler(profileStateChangeHandlerRef);
    le_mcc_profile_RemoveCallEventHandler(profileCallEventHandlerRef);
    le_mcc_profile_Release(profileRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Create and start a call.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_mcc_Call()
{
    le_result_t                              res = LE_FAULT;
    le_mcc_profile_ObjRef_t                  profileRef;
    le_mcc_profile_StateChangeHandlerRef_t   profileStateChangeHandlerRef;

    profileRef=le_mcc_profile_GetByName("Modem-Sim1");
    CU_ASSERT_PTR_NOT_NULL(profileRef);

    TestCallRef=le_mcc_profile_CreateCall(profileRef, DestinationNmbr);
    CU_ASSERT_PTR_NOT_NULL(TestCallRef);

    profileStateChangeHandlerRef = le_mcc_profile_AddStateChangeHandler(profileRef,
                                                                        MyProfileStateChangeHandler,
                                                                        NULL);
    CU_ASSERT_PTR_NOT_NULL(profileStateChangeHandlerRef);

    le_mcc_profile_AddCallEventHandler(profileRef, MyCallEventHandler, NULL);

    res = le_mcc_call_Start(TestCallRef);
    CU_ASSERT_EQUAL(res, LE_OK);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Expect an incoming call and then hang-up.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_mcc_HangUpAll()
{
    // Set a hang-up timer.
    HangUpTimer = le_timer_Create("HangUp");

    le_clk_Time_t interval = {10, 0};
    LE_ASSERT(le_timer_SetInterval(HangUpTimer, interval) == LE_OK);
    LE_ASSERT(le_timer_SetHandler(HangUpTimer, HangUpTimerHandler) == LE_OK);
}



