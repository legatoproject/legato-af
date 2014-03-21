 /**
  * This module implements the le_mcc's unit tests.
  *
  *
  * Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
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

#include "le_mcc.h"
#include "main.h"





#ifndef AUTOMATIC
static char  DEST_TEST_PATTERN[LE_TEL_NMBR_MAX_LEN];
#else
#define DEST_TEST_PATTERN  "XXXXXXXXXXXX"
#endif

static le_mcc_call_Ref_t       testCallRef;



#ifndef AUTOMATIC
//--------------------------------------------------------------------------------------------------
/**
 * This function gets the telephone number from the User (interactive case).
 *
 */
//--------------------------------------------------------------------------------------------------
void getTel(void)
{
    char *strPtr;

    do
    {
        fprintf(stderr, "Please enter the destination's telephone number to perform the MCC tests: \n");
        strPtr=fgets ((char*)DEST_TEST_PATTERN, 16, stdin);
    }while (strlen(strPtr) == 0);

    DEST_TEST_PATTERN[strlen(DEST_TEST_PATTERN)-1]='\0';
}
#endif



//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Profile State Change Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyProfileStateChangeHandler(le_mcc_profile_State_t  newState, void* contextPtr)
{
//     le_mcc_profile_State_t *locState = (le_mcc_profile_State_t*)newState;
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
static void MyCallEventHandler(le_mcc_call_Ref_t  callRef, le_mcc_call_Event_t callEvent, void* contextPtr)
{
    le_result_t         res;

    LE_INFO("MCC TEST: New Call event: %d for Call %p", callEvent, callRef);

    if (callEvent == LE_MCC_EVENT_ALERTING)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_ALERTING.");
    }
    else if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_CONNECTED.");
    }
    else if (callEvent == LE_MCC_EVENT_TERMINATED)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_TERMINATED.");
        le_mcc_call_TerminationReason_t term = le_mcc_call_GetTerminationReason(callRef);
        switch(term)
        {
            case LE_MCC_TERM_NETWORK_FAIL:
                LE_INFO("Termination reason is LE_MCC_TERM_NETWORK_FAIL");
                break;

            case LE_MCC_TERM_BAD_ADDRESS:
                LE_INFO("Termination reason is LE_MCC_TERM_BAD_ADDRESS");
                break;

            case LE_MCC_TERM_BUSY:
                LE_INFO("Termination reason is LE_MCC_TERM_BUSY");
                break;

            case LE_MCC_TERM_LOCAL_ENDED:
                LE_INFO("Termination reason is LE_MCC_TERM_LOCAL_ENDED");
                break;

            case LE_MCC_TERM_REMOTE_ENDED:
                LE_INFO("Termination reason is LE_MCC_TERM_REMOTE_ENDED");
                break;

            case LE_MCC_TERM_NOT_DEFINED:
                LE_INFO("Termination reason is LE_MCC_TERM_NOT_DEFINED");
                break;

            default:
                LE_INFO("Termination reason is %d", term);
                break;
        }
        le_mcc_call_Delete(callRef);
    }
    else if (callEvent == LE_MCC_EVENT_INCOMING)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_INCOMING.");
        res = le_mcc_call_Answer(callRef);
        if (res == LE_OK)
        {
            LE_INFO("Check MyCallEventHandler passed, I answered the call");
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
//                                       Test Functions
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Test: Modem Profile.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_mcc_Profile()
{
    le_mcc_profile_Ref_t                     profileRef;
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
    le_mcc_profile_Ref_t                     profileRef;
    le_mcc_profile_StateChangeHandlerRef_t   profileStateChangeHandlerRef;

    profileRef=le_mcc_profile_GetByName("Modem-Sim1");
    CU_ASSERT_PTR_NOT_NULL(profileRef);

    testCallRef=le_mcc_profile_CreateCall(profileRef, DEST_TEST_PATTERN);
    CU_ASSERT_PTR_NOT_NULL(testCallRef);

    profileStateChangeHandlerRef = le_mcc_profile_AddStateChangeHandler(profileRef,
                                                                        MyProfileStateChangeHandler,
                                                                        NULL);
    CU_ASSERT_PTR_NOT_NULL(profileStateChangeHandlerRef);

    le_mcc_profile_AddCallEventHandler(profileRef, MyCallEventHandler, NULL);

    res = le_mcc_call_Start(testCallRef);
    CU_ASSERT_EQUAL(res, LE_OK);
}





