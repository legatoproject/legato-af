 /**
  * This module implements the le_mcc's tests.
  *
  *
  * Copyright (C) Sierra Wireless Inc.
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
} myStruct;

//--------------------------------------------------------------------------------------------------
/**
 * CLIR field valid or not.
 */
//--------------------------------------------------------------------------------------------------
static bool ClirValid = true;

//--------------------------------------------------------------------------------------------------
/**
 * Calling Line Identification Restriction setting for the destination phone number to used.
 */
//--------------------------------------------------------------------------------------------------
static le_onoff_t ClirStatus = LE_OFF;

//--------------------------------------------------------------------------------------------------
/**
 * Call Reference.
 */
//--------------------------------------------------------------------------------------------------
static le_mcc_CallRef_t TestCallRef;

//--------------------------------------------------------------------------------------------------
/**
 * Hang Up Timer Reference.
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t HangUpTimer;

//--------------------------------------------------------------------------------------------------
/**
 * Destination Phone number.
 */
//--------------------------------------------------------------------------------------------------
static char  DestinationNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];

//--------------------------------------------------------------------------------------------------
/**
 * The audio AMR Wideband capability
 */
//--------------------------------------------------------------------------------------------------
static bool AmrWbCapState = true;

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
    LE_ERROR_IF(le_mcc_HangUpAll() != LE_OK, "Could not hangup.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Call Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyCallEventHandler
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t callEvent,
    void* contextPtr
)
{
    le_result_t         res;
    static bool firstConnectCall = true;

    LE_INFO("MCC TEST: New Call event: %d for Call %p", callEvent, callRef);

    if (callEvent == LE_MCC_EVENT_ALERTING)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_ALERTING.");
        if (firstConnectCall)
        {
            LE_INFO("---!!!! PLEASE CHECK ON THE REMOTE SIDE IF THE PHONE NUMBER IS %s !!!!---",
                            ((ClirStatus == LE_ON) ? "HIDED" : "DISPLAYED"));
            LE_INFO("---!!!! PLEASE HANG UP ON THE REMOTE SIDE !!!!---");
        }
    }
    else if (callEvent == LE_MCC_EVENT_CONNECTED)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_CONNECTED.");
        if (firstConnectCall)
        {
            LE_INFO("---!!!! PLEASE TERMINATE THE CALL on THE REMOTE SIDE !!!!---");
            firstConnectCall = false;
        }
    }
    else if (callEvent == LE_MCC_EVENT_TERMINATED)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_TERMINATED.");
        le_mcc_TerminationReason_t term = le_mcc_GetTerminationReason(callRef);
        int32_t code = le_mcc_GetPlatformSpecificTerminationCode(callRef);
        switch(term)
        {
            case LE_MCC_TERM_NETWORK_FAIL:
                LE_ERROR("Termination reason is LE_MCC_TERM_NETWORK_FAIL");
                exit(EXIT_FAILURE);
                break;

            case LE_MCC_TERM_UNASSIGNED_NUMBER:
                LE_ERROR("Termination reason is LE_MCC_TERM_UNASSIGNED_NUMBER");
                exit(EXIT_FAILURE);
                break;

            case LE_MCC_TERM_USER_BUSY:
                LE_ERROR("Termination reason is LE_MCC_TERM_USER_BUSY");
                exit(EXIT_FAILURE);
                break;

            case LE_MCC_TERM_LOCAL_ENDED:
                LE_INFO("Termination reason is LE_MCC_TERM_LOCAL_ENDED");
                LE_INFO("mccTest Sequence SUCCESS");
                LE_INFO("mccTest test exit");
                le_mcc_Delete(callRef);
                exit(EXIT_SUCCESS);
                break;

            case LE_MCC_TERM_REMOTE_ENDED:
                LE_INFO("Termination reason is LE_MCC_TERM_REMOTE_ENDED");
                LE_INFO("---!!!! PLEASE CREATE AN INCOMING CALL !!!!---");
                break;

            case LE_MCC_TERM_NO_SERVICE:
                LE_INFO("Termination reason is LE_MCC_TERM_NO_SERVICE");
                exit(EXIT_FAILURE);
                break;

            case LE_MCC_TERM_OPERATOR_DETERMINED_BARRING:
                LE_INFO("Termination reason is LE_MCC_TERM_OPERATOR_DETERMINED_BARRING");
                exit(EXIT_FAILURE);
                break;

            case LE_MCC_TERM_UNDEFINED:
                LE_ERROR("Termination reason is LE_MCC_TERM_UNDEFINED");
                LE_ERROR("---!!!! PLEASE CREATE AN INCOMING CALL !!!!---");
                break;

            default:
                LE_ERROR("Termination reason is %d", term);
                exit(EXIT_FAILURE);
                break;
        }
        LE_INFO("Termination code is 0x%X", code);

        if (HangUpTimer)
        {
            le_timer_Stop(HangUpTimer);
        }
    }
    else if (callEvent == LE_MCC_EVENT_INCOMING)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_INCOMING.");
        res = le_mcc_Answer(callRef);
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
    else if (callEvent == LE_MCC_EVENT_ORIGINATING)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_ORIGINATING.");
    }
    else if (callEvent == LE_MCC_EVENT_SETUP)
    {
        LE_INFO("Check MyCallEventHandler passed, event is LE_MCC_EVENT_SETUP.");
    }
    else
    {
        LE_ERROR("Check MyCallEventHandler failed, unknowm event %d.", callEvent);
    }
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
    le_result_t          res = LE_FAULT;
    le_onoff_t           localClirStatus;
    bool                 amrWbCapabilityState;

    TestCallRef = le_mcc_Create(DestinationNumber);

    if (TestCallRef == NULL)
    {
        return LE_FAULT;
    }

    le_mcc_AddCallEventHandler(MyCallEventHandler, NULL);

    if (ClirValid == true)
    {
        res = le_mcc_SetCallerIdRestrict(TestCallRef, ClirStatus);
        if (res != LE_OK)
        {
            LE_ERROR("Failed to set Caller Id Restriction");
            return res;
        }
    }

    res = le_mcc_GetCallerIdRestrict(TestCallRef, &localClirStatus);
    if (((ClirValid == true) && (res != LE_OK)) ||
        ((ClirValid == false) && (res != LE_OK) && (res != LE_UNAVAILABLE)))
    {
        LE_ERROR("Failed to get Caller Id Restriction");
        return res;
    }

    if ((res == LE_OK) && (localClirStatus !=  ClirStatus))
    {
        LE_ERROR("CLIR status doesn't match with CLIR set");
        return LE_FAULT;
    }

    if (LE_OK != le_mcc_SetAmrWbCapability(AmrWbCapState))
    {
        LE_ERROR("Set AMR Wideband capability Error");
        return LE_FAULT;
    }

    if (LE_OK != le_mcc_GetAmrWbCapability(&amrWbCapabilityState))
    {
        LE_ERROR("Get AMR Wideband capability Error");
        return LE_FAULT;
    }

    if (amrWbCapabilityState != AmrWbCapState)
    {
        LE_ERROR("AMR Wideband capability Error");
        return LE_FAULT;
    }

    res = le_mcc_Start(TestCallRef);
    if (res != LE_OK)
    {
       le_mcc_TerminationReason_t reason = le_mcc_GetTerminationReason(TestCallRef);

       switch(reason)
       {
           case LE_MCC_TERM_FDN_ACTIVE:
               LE_ERROR("Term reason LE_MCC_TERM_FDN_ACTIVE");
               break;
           case LE_MCC_TERM_NOT_ALLOWED:
               LE_ERROR("Term reason LE_MCC_TERM_NOT_ALLOWED");
               break;
           case LE_MCC_TERM_UNDEFINED:
               LE_ERROR("Term reason LE_MCC_TERM_UNDEFINED");
               break;
           default:
               LE_ERROR("Term reason %d", reason);
       }
       return LE_FAULT;
    }
    else
    {
        res = le_mcc_Start(TestCallRef);
        LE_ASSERT(res == LE_BUSY);
        LE_INFO("le_mcc_Start() LE_BUSY test OK");
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
 * Test init
 *
 * - ME must be registered on Network with the SIM in ready state.
 * - According to PCM or I2S configuration and the type of board, execute the shell commands:
 *   PCM: for AR755x, AR8652 devkit's codec use, "wm8940_demo --pcm" (not supported on mangOH board)
 *   I2S: for AR755x, AR8652 devkit's codec use, "wm8940_demo --i2s" (not supported on mangOH board)
 * - Check "logread -f | grep mcc" log
 * - Start app : app start mccTest
 * - Execute app :
 *  app runProc mccTest --exe=mccTest -- <Destination phone number> <CLIR_ON | CLIR_OFF | NO_CLIR>
 *  <AMR_WB_ENABLE | AMR_WB_DISABLE>
 *   - CLIR_ON to activate the Calling line identification restriction. Phone Number is not
 * displayed on the remote side.
 *   - CLIR_OFF to deactivate the Calling line identification restriction. Phone Number can be
 * displayed on the remote side.
 *   - NO_CLIR to indicate not to set the Calling line identification restriction for this call.
 *   - AMR_WB_DISABLE disables the audio AMR Wideband capability.
 *   - AMR_WB_ENABLE enables the audio AMR Wideband capability.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    int i, nbArgument = 0;
    le_result_t res;
    myStruct mcctest[] =
    {
                    { "Test le_mcc_Call()",         Testle_mcc_Call },
                    { "Test le_mcc_HangUpAll()",    Testle_mcc_HangUpAll },
                    { "", NULL }
    };

    nbArgument = le_arg_NumArgs();

    if (3 == nbArgument)
    {
        // This function gets the telephone number from the User (interactive case).
        const char* phoneNumber = le_arg_GetArg(0);
        if (NULL == phoneNumber)
        {
            LE_ERROR("phoneNumber is NULL");
            exit(EXIT_FAILURE);
        }
        const char* clirStatusStr = le_arg_GetArg(1);
        strncpy(DestinationNumber, phoneNumber, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);

        if (clirStatusStr)
        {
           if (0 == strcmp(clirStatusStr, "NO_CLIR"))
           {
               ClirValid = false;
               LE_INFO("Phone number %s, No CLIR", DestinationNumber);
           }
           else if (0 == strcmp(clirStatusStr, "CLIR_ON"))
           {
               ClirValid = true;
               ClirStatus = LE_ON;
               LE_INFO("Phone number %s, CLIR ON", DestinationNumber);
           }
           else if (0 == strcmp(clirStatusStr, "CLIR_OFF"))
           {
               ClirValid = true;
               ClirStatus = LE_OFF;
               LE_INFO("Phone number %s, CLIR OFF", DestinationNumber);
           }
           else
           {
               LE_ERROR("Incorrect argument '%s'", clirStatusStr);
               exit(EXIT_FAILURE);
           }
        }

        const char* amrWbCapStr = le_arg_GetArg(2);

        if (amrWbCapStr)
        {
           if (0 == strcmp(amrWbCapStr, "AMR_WB_DISABLE"))
           {
               AmrWbCapState = false;
           }
           else if (0 == strcmp(amrWbCapStr, "AMR_WB_ENABLE"))
           {
               AmrWbCapState = true;
           }
           else
           {
               LE_ERROR("Incorrect AMR Widband Capability argument '%s'", amrWbCapStr);
               exit(EXIT_FAILURE);
           }
        }

        for (i=0; mcctest[i].ptrfunc != NULL; i++)
        {
            LE_INFO("Test %s STARTED\n", mcctest[i].name);
            res = mcctest[i].ptrfunc();
            if (LE_OK != res)
            {
                LE_ERROR("Test %s FAILED\n", mcctest[i].name);
                LE_INFO("mccTest sequence FAILED");
                exit(EXIT_FAILURE);
            }
            LE_INFO("Test %s PASSED\n", mcctest[i].name);
        }
    }
    else
    {
        LE_ERROR("PRINT USAGE => app runProc mccTest --exe=mccTest -- <Destination phone number>"
                        " <CLIR_ON | CLIR_OFF | NO_CLIR> <AMR_WB_ENABLE | AMR_WB_DISABLE>");
        exit(EXIT_SUCCESS);
    }
}
