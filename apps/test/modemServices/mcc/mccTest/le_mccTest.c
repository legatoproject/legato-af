 /**
  * This module implements the le_mcc's unit tests.
  *
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Test sequence Structure list
 */
//--------------------------------------------------------------------------------------------------

typedef le_result_t (*TestFunc)(void);

typedef struct
{
        char * name;
        TestFunc ptrfunc;
} my_struct;


static le_mcc_call_ObjRef_t TestCallRef;
static le_timer_Ref_t HangUpTimer;
static char  DestinationNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];

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
static void MyProfileStateChangeHandler
(
    le_mcc_profile_State_t  newState,
    void* contextPtr
)
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
static void MyCallEventHandler
(
    le_mcc_call_ObjRef_t  callRef,
    le_mcc_call_Event_t callEvent,
    void* contextPtr
)
{
    le_result_t         res;
    static bool firstConnectCall = true;

    LE_INFO("MCC TEST: New Call event: %d for Call %p", callEvent, callRef);

    if (callEvent == LE_MCC_CALL_EVENT_ALERTING)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_CALL_EVENT_ALERTING.");
        if (firstConnectCall)
        {
            LE_INFO("---!!!! PLEASE HANG UP ON THE REMOTE SIDE !!!!---");
        }
    }
    else if (callEvent == LE_MCC_CALL_EVENT_CONNECTED)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_CALL_EVENT_CONNECTED.");
        if (firstConnectCall)
        {
            LE_INFO("---!!!! PLEASE TERMINATE THE CALL on THE REMOTE SIDE !!!!---");
            firstConnectCall = false;
        }
    }
    else if (callEvent == LE_MCC_CALL_EVENT_TERMINATED)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_CALL_EVENT_TERMINATED.");
        le_mcc_call_TerminationReason_t term = le_mcc_call_GetTerminationReason(callRef);
        int32_t code = le_mcc_call_GetPlatformSpecificTerminationCode(callRef);
        switch(term)
        {
            case LE_MCC_CALL_TERM_NETWORK_FAIL:
                LE_ERROR("Termination reason is LE_MCC_CALL_TERM_NETWORK_FAIL");
                exit(EXIT_FAILURE);
                break;

            case LE_MCC_CALL_TERM_UNASSIGNED_NUMBER:
                LE_ERROR("Termination reason is LE_MCC_CALL_TERM_UNASSIGNED_NUMBER");
                exit(EXIT_FAILURE);
                break;

            case LE_MCC_CALL_TERM_USER_BUSY:
                LE_ERROR("Termination reason is LE_MCC_CALL_TERM_USER_BUSY");
                exit(EXIT_FAILURE);
                break;

            case LE_MCC_CALL_TERM_LOCAL_ENDED:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_LOCAL_ENDED");
                LE_INFO("mccTest Sequence SUCCESS");
                LE_INFO("mccTest test exit");
                exit(EXIT_SUCCESS);
                break;

            case LE_MCC_CALL_TERM_REMOTE_ENDED:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_REMOTE_ENDED");
                LE_INFO("---!!!! PLEASE CREATE AN INCOMING CALL !!!!---");
                break;

            case LE_MCC_CALL_TERM_UNDEFINED:
                LE_ERROR("Termination reason is LE_MCC_CALL_TERM_UNDEFINED");
                LE_ERROR("---!!!! PLEASE CREATE AN INCOMING CALL !!!!---");
                break;

            default:
                LE_ERROR("Termination reason is %d", term);
                exit(EXIT_FAILURE);
                break;
        }
        LE_INFO("Termination code is 0x%X", code);

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
            LE_ERROR("Check MyCallEventHandler failed to answer the call.");
        }
    }
    else
    {
        LE_ERROR("Check MyCallEventHandler failed, unknowm event.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Modem Profile.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_mcc_Profile
(
    void
)
{
    le_mcc_profile_ObjRef_t                  profileRef;
    le_mcc_profile_State_t                   profileState;
    le_mcc_profile_StateChangeHandlerRef_t   profileStateChangeHandlerRef;
    le_mcc_profile_CallEventHandlerRef_t     profileCallEventHandlerRef;

    profileRef = le_mcc_profile_GetByName("Modem-Sim1");
    if (profileRef == NULL)
    {
        return LE_FAULT;
    }

    profileState = le_mcc_profile_GetState(profileRef);
    //CU_ASSERT_TRUE((profileState>=LE_MCC_PROFILE_NOT_AVAILABLE) && (profileState<=LE_MCC_PROFILE_IN_USE));
    if( ( (profileState >= LE_MCC_PROFILE_NOT_AVAILABLE)
           && (profileState <= LE_MCC_PROFILE_IN_USE) ) != true)
    {
        return LE_FAULT;
    }

    profileStateChangeHandlerRef = le_mcc_profile_AddStateChangeHandler(profileRef,
                    MyProfileStateChangeHandler,
                    NULL);
    if (profileStateChangeHandlerRef == NULL)
    {
        return LE_FAULT;
    }

    profileCallEventHandlerRef = le_mcc_profile_AddCallEventHandler(profileRef, MyCallEventHandler, NULL);

    le_mcc_profile_RemoveStateChangeHandler(profileStateChangeHandlerRef);
    le_mcc_profile_RemoveCallEventHandler(profileCallEventHandlerRef);
    le_mcc_profile_Release(profileRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Create and start a call.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_mcc_Call
(
    void
)
{
    le_result_t                              res = LE_FAULT;
    le_mcc_profile_ObjRef_t                  profileRef;
    le_mcc_profile_StateChangeHandlerRef_t   profileStateChangeHandlerRef;

    profileRef = le_mcc_profile_GetByName("Modem-Sim1");
    if (profileRef == NULL)
    {
        return LE_FAULT;
    }

    TestCallRef = le_mcc_profile_CreateCall(profileRef, DestinationNumber);
    if (TestCallRef == NULL)
    {
        return LE_FAULT;
    }

    profileStateChangeHandlerRef = le_mcc_profile_AddStateChangeHandler(profileRef,
                    MyProfileStateChangeHandler,
                    NULL);
    if (profileStateChangeHandlerRef == NULL)
    {
        return LE_FAULT;
    }

    le_mcc_profile_AddCallEventHandler(profileRef, MyCallEventHandler, NULL);

    res = le_mcc_call_Start(TestCallRef);
    if (res != LE_OK)
    {
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Expect an incoming call and then hang-up.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Testle_mcc_HangUpAll
(
    void
)
{
    // Set a hang-up timer.
    HangUpTimer = le_timer_Create("HangUp");

    le_clk_Time_t interval = {10, 0};
    LE_ASSERT(le_timer_SetInterval(HangUpTimer, interval) == LE_OK);
    LE_ASSERT(le_timer_SetHandler(HangUpTimer, HangUpTimerHandler) == LE_OK);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/*
 * ME must be registered on Network with the SIM in ready state.
 * Check "logread -f | grep mcc" log
 * Start app : app start mccTest
 * Execute app : execInApp mccTest mccTest <Destination phone number>
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    int i;
    le_result_t res;

    my_struct mcctest[] =
    {
                    { "Test le_mcc_Profile()",      Testle_mcc_Profile },
                    { "Test le_mcc_Call()",         Testle_mcc_Call },
                    { "Test le_mcc_HangUpAll()",    Testle_mcc_HangUpAll },
                    { "", NULL }
    };

    if (le_arg_NumArgs() == 1)
    {
        // This function gets the telephone number from the User (interactive case).
        const char* phoneNumber = le_arg_GetArg(0);
        strncpy(DestinationNumber, phoneNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
        LE_INFO("Phone number %s", DestinationNumber);

        for (i=0; mcctest[i].ptrfunc != NULL; i++)
        {
            LE_INFO("Test %s STARTED\n", mcctest[i].name);
            res =  mcctest[i].ptrfunc();
            if (res != LE_OK)
            {
                LE_ERROR("Test %s FAILED\n", mcctest[i].name);
                LE_INFO("mccTest sequence FAILED");
                exit(EXIT_FAILURE);
            }
            else
            {
                LE_INFO("Test %s PASSED\n", mcctest[i].name);
            }
        }
    }
    else
    {
        LE_ERROR("PRINT USAGE => execInApp mccTest mccTest <Destination phone number>");
        exit(EXIT_SUCCESS);
    }
}
