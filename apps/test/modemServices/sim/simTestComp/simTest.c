 /**
  * This module implements the le_simRef's unit tests.
  *
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include <stdio.h>
#include <semaphore.h>

#include "interfaces.h"
#include "main.h"

// TODO: retrieve PIN and PUK from configuration in automatic mode
#define PIN_TEMP            "0123"
#define PIN_TEST1           "1111"
#define PIN_TEST2           "2222"
#define NEW_PIN_TEST        "5678"
#define FAIL_PIN_TEST       "4321"
#define PIN_TOO_LONG_TEST   "123456789"
#define PIN_TOO_SHORT_TEST  "12"
#define PUK_TEST1           "11111111"
#define PUK_TEST2           "22222222"
#define FAIL_PUK_TEST       "87654321"
#define PUK_BAD_LENGTH_TEST "12"



//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SIM States Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSimStateHandler(le_sim_ObjRef_t simRef, void* contextPtr)
{
    le_sim_States_t       state;
    uint32_t              slot=le_sim_GetSlotNumber(simRef);

    // Get SIM state
    state = le_sim_GetState(simRef);

    switch(state)
    {
        case LE_SIM_INSERTED:
            LE_INFO("-TEST- New state LE_SIM_INSERTED for SIM card.%d", slot);
            break;
        case LE_SIM_ABSENT:
            LE_INFO("-TEST- New state LE_SIM_ABSENT for SIM card.%d", slot);
            break;
        case LE_SIM_READY:
            LE_INFO("-TEST- New state LE_SIM_READY for SIM card.%d", slot);
            break;
        case LE_SIM_BLOCKED:
            LE_INFO("-TEST- New state LE_SIM_BLOCKED for SIM card.%d", slot);
            break;
        case LE_SIM_BUSY:
            LE_INFO("-TEST- New state LE_SIM_BUSY for SIM card.%d", slot);
            break;
        case LE_SIM_STATE_UNKNOWN:
            LE_INFO("-TEST- New state LE_SIM_STATE_UNKNOWN for SIM card.%d", slot);
            break;
        default:
            LE_INFO("-TEST- New state %d for SIM card.%d", state, slot);
            break;
    }
    if((state>=LE_SIM_INSERTED) && (state<=LE_SIM_STATE_UNKNOWN))
    {
        LE_INFO("-TEST- Check le_sim_GetState passed.");
    }
    else
    {
        LE_ERROR("-TEST- Check le_sim_GetState failure !");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function display the SIM state.
 *
 */
//--------------------------------------------------------------------------------------------------
static void displaySIMState(le_sim_States_t state, uint32_t slot)
{
    char string[100];

    memset(string,0,100);

    switch(state)
    {
        case LE_SIM_INSERTED:
            sprintf(string, "\nSIM Card state LE_SIM_INSERTED for SIM card.%d \n", slot);
            break;
        case LE_SIM_ABSENT:
            sprintf(string, "\nSIM Card state LE_SIM_ABSENT for SIM card.%d \n", slot);
            break;
        case LE_SIM_READY:
            sprintf(string, "\nSIM Card state LE_SIM_READY for SIM card.%d \n", slot);
            break;
        case LE_SIM_BLOCKED:
            sprintf(string, "\nSIM Card state LE_SIM_BLOCKED for SIM card.%d \n", slot);
            break;
        case LE_SIM_BUSY:
            sprintf(string, "\nSIM Card state LE_SIM_BUSY for SIM card.%d \n", slot);
            break;
        case LE_SIM_STATE_UNKNOWN:
            sprintf(string, "\nSIM Card state LE_SIM_STATE_UNKNOWN for SIM card.%d \n", slot);
            break;
        default:
            sprintf(string, "\nSIM Card state %d for SIM card.%d \n", state, slot);
            break;
    }

    Print(string);
}


//--------------------------------------------------------------------------------------------------
//                                       Test Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Test: Creation & information retrieving.
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_Create
(
    le_sim_Type_t cardNum,
    const char* pinPtr
)
{
    bool            presence=false;
    le_sim_ObjRef_t simRef;
    char            iccid[LE_SIM_ICCID_BYTES] = {0};
    char            imsi[LE_SIM_IMSI_BYTES] = {0};
    le_result_t     res;

    // Get the handle on the requested SIM
    simRef = le_sim_Create(cardNum);
    LE_ASSERT(simRef!=NULL);

    // Enter PIN code
    res = le_sim_EnterPIN(simRef, pinPtr);
    LE_ASSERT(res==LE_OK);

    // Get ICCID
    res = le_sim_GetICCID(simRef, iccid, sizeof(iccid));
    LE_ASSERT(res==LE_OK);
    Print( iccid );

    // Get IMSI
    res = le_sim_GetIMSI(simRef, imsi, sizeof(imsi));
    LE_ASSERT(res==LE_OK);
    Print( imsi );

    // Check if SIM present
    presence=le_sim_IsPresent(simRef);
    LE_ASSERT(presence);

    // Unsubscribe to the SIM
    le_sim_Delete(simRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: SIM lock/unlock.
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_Lock
(
    le_sim_Type_t cardNum,
    const char* pinPtr
)
{
    le_sim_States_t state;
    le_sim_ObjRef_t simRef;
    bool doLock;
    uint8_t loop=0;
    le_result_t     res;

    // Get the handle on the requested SIM
    simRef = le_sim_Create(cardNum);
    LE_ASSERT(simRef!=NULL);

    // Get SIM state
    state = le_sim_GetState(simRef);
    displaySIMState(state, cardNum);
    LE_ASSERT((state == LE_SIM_READY)||(state == LE_SIM_INSERTED));

    if(state == LE_SIM_READY)
    {
        doLock = true;
    }
    else if (state == LE_SIM_INSERTED)
    {
        // Enter PIN code
        res = le_sim_EnterPIN(simRef, pinPtr);
        LE_ASSERT(res==LE_OK);
        doLock = false;
    }

    // Try to lock/unlock the SIM
    while (loop<2)
    {
        LE_INFO("loop %d doLock %d", loop, doLock);

        if (doLock)
        {
            LE_INFO("lock the SIM");

            // Lock PIN using a wrong PIN code (error code expected)
            res = le_sim_Lock(simRef, FAIL_PIN_TEST);
            LE_ASSERT(res==LE_FAULT);

            // Lock PIN using the correct PIN code
            res = le_sim_Lock(simRef, pinPtr);
            LE_ASSERT(res==LE_OK);

            // Enter PIN code
            res = le_sim_EnterPIN(simRef, pinPtr);
            LE_ASSERT(res==LE_OK);
            doLock = false;
        }
        else
        {
            LE_INFO("unlock the SIM");

            // Unlock the SIM using a wrong PIN code (error code expected)
            res = le_sim_Unlock(simRef, FAIL_PIN_TEST);
            LE_ASSERT(res==LE_FAULT);

            // Lock PIN using the correct PIN code
            res = le_sim_Unlock(simRef, pinPtr);
            LE_ASSERT(res==LE_OK);
            doLock=true;
        }

        loop++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Authentication (pin/puk).
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_Authentication
(
    le_sim_Type_t cardNum,
    const char* pinPtr,
    const char* pukPtr
)
{
    le_result_t     res;
    bool            ready=false;
    int32_t         initTries=0;
    int32_t         tries=0;
    le_sim_ObjRef_t simRef;
    char            string[100];

    memset(string,0,100);

    // Get the handle on the requested SIM
    simRef = le_sim_Create(cardNum);
    LE_ASSERT(simRef!=NULL);

    // Get the remaining PIN entries
    initTries=le_sim_GetRemainingPINTries(simRef);

    // Enter PIN code
    res = le_sim_EnterPIN(simRef, FAIL_PIN_TEST);
    LE_ASSERT(res==LE_FAULT);

    // Get the remaining PIN entries
    tries=le_sim_GetRemainingPINTries(simRef);
    LE_ASSERT((initTries-tries) == 1);

    // Check that the SIM is not ready
    ready=le_sim_IsReady(simRef);
    LE_ASSERT(!ready);

    // Enter PIN code
    res = le_sim_EnterPIN(simRef, pinPtr);
    LE_ASSERT(res==LE_OK);

    // Check that the SIM is ready
    ready=le_sim_IsReady(simRef);
    LE_ASSERT(ready);

    // Change PIN
    res = le_sim_ChangePIN(simRef, FAIL_PIN_TEST, NEW_PIN_TEST);
    LE_ASSERT(res==LE_FAULT);

    // Change the PIN code
    res = le_sim_ChangePIN(simRef, pinPtr, NEW_PIN_TEST);
    LE_ASSERT(res==LE_OK);

    // block the SIM:
    // while remaining PIN entries not null, enter a wrong PIN code
    while((initTries=le_sim_GetRemainingPINTries(simRef))>0)
    {
        // Enter PIN code
        res = le_sim_EnterPIN(simRef, FAIL_PIN_TEST);
    }

    if(initTries < 0)
    {
        sprintf(string, "\nle_sim_GetRemainingPINTries error, res.%d (should be >=0)\n", initTries);
        Print(string);
    }

    // Unblock the SIM using a wrong PUK code (error expected)
    res = le_sim_Unblock(simRef, FAIL_PUK_TEST, NEW_PIN_TEST);
    LE_ASSERT(res==LE_FAULT);

    // Unblock the SIM using the correct PUK code
    res = le_sim_Unblock(simRef, pukPtr, NEW_PIN_TEST);
    LE_ASSERT(res==LE_OK);

    // Unsubscribe to the SIM
    le_sim_Delete(simRef);

    Print("End simTest_Authentication");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: SIM absent.
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_SimAbsent
(
    le_sim_Type_t cardNum
)
{
    le_sim_ObjRef_t simRef;
    int32_t         initTries=0;
    le_result_t     res;
    bool            ready=false;

    // Get the handle on the requested SIM
    simRef = le_sim_Create(cardNum);
    LE_ASSERT(simRef!=NULL);

    // Get the remaining PIN entries (error expected as no SIM)
    initTries=le_sim_GetRemainingPINTries(simRef);
    LE_ASSERT((initTries == LE_NOT_FOUND) || (initTries == LE_FAULT));

    // Enter PIN code (error expected as no SIM)
    res = le_sim_EnterPIN(simRef, PIN_TEMP);
    LE_ASSERT((res == LE_NOT_FOUND) || (res == LE_FAULT));

    // Check that the SIM is not ready
    ready=le_sim_IsReady(simRef);
    LE_ASSERT(!ready);

    // Change PIN (error expected as no SIM)
    res = le_sim_ChangePIN(simRef, PIN_TEMP, NEW_PIN_TEST);
    LE_ASSERT((res == LE_NOT_FOUND) || (res == LE_FAULT));

    // Unblock PIN  (error expected as no SIM)
    res = le_sim_Unblock(simRef, (char*)PUK_TEST1, PIN_TEMP);
    LE_ASSERT((res == LE_NOT_FOUND) || (res == LE_FAULT));

    // Unlock PIN  (error expected as no SIM)
    res = le_sim_Unlock(simRef, PIN_TEMP);
    LE_ASSERT((res == LE_NOT_FOUND) || (res == LE_FAULT));

    // Unsubscribe to the SIM
    le_sim_Delete(simRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: SIM selection.
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_SimSelect
(
)
{
    // Get the handle on the external SIM
    le_sim_ObjRef_t simExtRef = le_sim_Create(LE_SIM_EXTERNAL_SLOT_1);
    LE_ASSERT(simExtRef!=NULL);

    // Get the handle on the embedded SIM
    le_sim_ObjRef_t simEmbeddedRef = le_sim_Create(LE_SIM_EMBEDDED);
    LE_ASSERT(simEmbeddedRef!=NULL);

    // Select the embedded SIM
    le_result_t res = le_sim_SelectCard(simEmbeddedRef);
    LE_ASSERT(res==LE_OK);

    // Get the selected card
    le_sim_Type_t cardNum = le_sim_GetSelectedCard();
    LE_ASSERT(cardNum==LE_SIM_EMBEDDED);

    // Select the embedded SIM
    res = le_sim_SelectCard(simExtRef);
    LE_ASSERT(res==LE_OK);

    // Get the selected card
    cardNum = le_sim_GetSelectedCard();
    LE_ASSERT(cardNum==LE_SIM_EXTERNAL_SLOT_1);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: SIM State.
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_State
(
    le_sim_Type_t cardNum,
    const char* pinPtr
)
{
    le_result_t                 res;
    le_sim_NewStateHandlerRef_t testHdlrRef;
    le_sim_States_t             state;
    le_sim_ObjRef_t             simRef;
    char                        string[100];

    memset(string, 0, 100);

    // Get the handle on the requested SIM
    simRef = le_sim_Create(cardNum);

    LE_ASSERT(simRef!=NULL);

    // Get SIM state
    state = le_sim_GetState(simRef);

    LE_ASSERT((state>=LE_SIM_INSERTED) && (state<=LE_SIM_BUSY));
    sprintf(string, "\nSIM Card.%d state:\n", cardNum);
    Print(string);

    displaySIMState(state, cardNum);

    if(state == LE_SIM_INSERTED)
    {
        // Enter PIN code
        res = le_sim_EnterPIN(simRef, pinPtr);
        LE_ASSERT(res==LE_OK);

        // Get SIM state
        state = le_sim_GetState(simRef);
        LE_ASSERT((state>=LE_SIM_INSERTED) && (state<=LE_SIM_BUSY));
    }

    // Unsubscribe to the SIM
    le_sim_Delete(simRef);

    // Add the state handler
    testHdlrRef=le_sim_AddNewStateHandler(TestSimStateHandler, NULL);
    LE_ASSERT(testHdlrRef!=NULL);
}


