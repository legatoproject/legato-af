 /**
  * This module implements the le_simRef's unit tests.
  *
  * TODO: retrieving of PIN and PUK from configuration is missing + Disable Legato DEBUG logs
  *
  * Copyright (C) Sierra Wireless, Inc. 2013-2014. Use of this work is subject to license.
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


static char    PIN_TEST[2][16];
static char    PUK_TEST[2][16];



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
    switch(state)
    {
        case LE_SIM_INSERTED:
            fprintf(stderr, "\nSIM Card state LE_SIM_INSERTED for SIM card.%d \n", slot);
            break;
        case LE_SIM_ABSENT:
            fprintf(stderr, "\nSIM Card state LE_SIM_ABSENT for SIM card.%d \n", slot);
            break;
        case LE_SIM_READY:
            fprintf(stderr, "\nSIM Card state LE_SIM_READY for SIM card.%d \n", slot);
            break;
        case LE_SIM_BLOCKED:
            fprintf(stderr, "\nSIM Card state LE_SIM_BLOCKED for SIM card.%d \n", slot);
            break;
        case LE_SIM_BUSY:
            fprintf(stderr, "\nSIM Card state LE_SIM_BUSY for SIM card.%d \n", slot);
            break;
        case LE_SIM_STATE_UNKNOWN:
            fprintf(stderr, "\nSIM Card state LE_SIM_STATE_UNKNOWN for SIM card.%d \n", slot);
            break;
        default:
            fprintf(stderr, "\nSIM Card state %d for SIM card.%d \n", state, slot);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the PIN and PUK codes from the User (interactive case).
 *
 */
//--------------------------------------------------------------------------------------------------
static void getCodes(void)
{
    uint32_t  i=1;
    char *strPtr;

    fprintf(stderr, "\n\n");
    do
    {
        do
        {
            fprintf(stderr, "Please enter the PIN code of SIM card.%d: \n",i);
            strPtr=fgets ((char*)&PIN_TEST[i-1][0], 16, stdin);
        }while (strlen(strPtr) == 0);
        PIN_TEST[i-1][strlen((char*)&PIN_TEST[i-1][0])-1] = '\0';

        do
        {
            fprintf(stderr, "Please enter the PUK code of SIM card.%d: \n",i);
            strPtr=fgets ((char*)&PUK_TEST[i-1][0], 16, stdin);
        }while (strlen(strPtr) == 0);
        PUK_TEST[i-1][strlen((char*)&PUK_TEST[i-1][0])-1] = '\0';
        i++;
    } while (i<=le_sim_CountSlots());
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
void Testle_sim_Create()
{
    bool            presence=false;
    le_sim_ObjRef_t simRef;
    char            iccid[LE_SIM_ICCID_BYTES] = {0};
    char            imsi[LE_SIM_IMSI_BYTES] = {0};
    uint32_t        i=1;
    le_result_t     res;

    do
    {
        simRef = le_sim_Create(i);
        CU_ASSERT_PTR_NOT_NULL(simRef);

        res = le_sim_GetICCID(simRef, iccid, sizeof(iccid));
        CU_ASSERT_EQUAL(res, LE_OK);

        res = le_sim_GetIMSI(simRef, imsi, sizeof(imsi));
        CU_ASSERT_EQUAL(res, LE_OK);

        presence=le_sim_IsPresent(simRef);
        CU_ASSERT_TRUE(presence);

        le_sim_Delete(simRef);
        i++;
    } while (i<=le_sim_CountSlots());
}



//--------------------------------------------------------------------------------------------------
/**
 * Test: Authentication (Interactive).
 *
 */
//--------------------------------------------------------------------------------------------------
void TestInteractivele_sim_Authentication()
{
    le_result_t     res;
    bool            ready=false;
    int32_t         initTries=0;
    int32_t         tries=0;
    le_sim_ObjRef_t simRef;
    uint32_t        i=1;
    bool            pinReq=true;
    char            internalPin[2][16];
    le_sim_States_t state;

    getCodes();

    do
    {
        fprintf(stderr, "\nTake off, then insert SIM card.%d, wait for +WIND:1 (approx. 2s) and then press enter \n", i);
        while ( getchar() != '\n' );

        simRef = le_sim_Create(i);
        CU_ASSERT_PTR_NOT_NULL(simRef);

        state = le_sim_GetState(simRef);
        displaySIMState(state, i);
        fprintf(stderr, "\nPress enter to continue...\n");
        while ( getchar() != '\n' );

        strcpy((char*)&internalPin[i-1][0], (char*)&PIN_TEST[i-1][0]);

        // Enter PIN
        if(state == LE_SIM_READY)
        {
            pinReq=false;
            // Lock PIN
            res = le_sim_Lock(simRef, PIN_TOO_LONG_TEST);
            CU_ASSERT_EQUAL(res, LE_OVERFLOW);
            res = le_sim_Lock(simRef, PIN_TOO_SHORT_TEST);
            CU_ASSERT_EQUAL(res, LE_UNDERFLOW);
            res = le_sim_Lock(simRef, FAIL_PIN_TEST);
            CU_ASSERT_EQUAL(res, LE_NOT_POSSIBLE);
            res = le_sim_Lock(simRef, (char*)&internalPin[i-1][0]);
            CU_ASSERT_EQUAL(res, LE_OK);
            fprintf(stderr, "\nle_sim_Lock, res.%d  (should be LE_OK=0)\n", res);
            fprintf(stderr, "\nTake off, then insert SIM card.%d, wait for +WIND:1 (approx. 2s) and then press enter \n", i);
            while ( getchar() != '\n' );
        }

        initTries=le_sim_GetRemainingPINTries(simRef);
        res = le_sim_EnterPIN(simRef, PIN_TOO_LONG_TEST);
        CU_ASSERT_EQUAL(res, LE_OVERFLOW);
        res = le_sim_EnterPIN(simRef, PIN_TOO_SHORT_TEST);
        CU_ASSERT_EQUAL(res, LE_UNDERFLOW);
        res = le_sim_EnterPIN(simRef, FAIL_PIN_TEST);
        CU_ASSERT_EQUAL(res, LE_NOT_POSSIBLE);

        tries=le_sim_GetRemainingPINTries(simRef);
        CU_ASSERT_TRUE((initTries-tries == 1));

        ready=le_sim_IsReady(simRef);
        CU_ASSERT_FALSE(ready);

        res = le_sim_EnterPIN(simRef, (char*)&internalPin[i-1][0]);
        CU_ASSERT_EQUAL(res, LE_OK);

        ready=le_sim_IsReady(simRef);
        CU_ASSERT_TRUE(ready);
#if 0
        if (ready != true)
        {
            LE_FATAL("SIM card.%d NOT READY ! check it, test exits !", i);
        }
#endif
        fprintf(stderr, "\nle_sim_EnterPIN, res.%d (should be LE_OK=0) \n", res);
        fprintf(stderr, "\nWait for SIM card.%d answer (+CREG: 1, approx. 2s) and then press enter \n", i);
        while ( getchar() != '\n' );

        // Change PIN
        res = le_sim_ChangePIN(simRef, PIN_TOO_LONG_TEST, NEW_PIN_TEST);
        CU_ASSERT_EQUAL(res, LE_OVERFLOW);
        res = le_sim_ChangePIN(simRef, (char*)&internalPin[i-1][0], PIN_TOO_LONG_TEST);
        CU_ASSERT_EQUAL(res, LE_OVERFLOW);
        res = le_sim_ChangePIN(simRef, PIN_TOO_SHORT_TEST, NEW_PIN_TEST);
        CU_ASSERT_EQUAL(res, LE_UNDERFLOW);
        res = le_sim_ChangePIN(simRef, (char*)&internalPin[i-1][0], PIN_TOO_SHORT_TEST);
        CU_ASSERT_EQUAL(res, LE_UNDERFLOW);
        res = le_sim_ChangePIN(simRef, FAIL_PIN_TEST, NEW_PIN_TEST);
        CU_ASSERT_EQUAL(res, LE_NOT_POSSIBLE);
        res = le_sim_ChangePIN(simRef, (char*)&internalPin[i-1][0], NEW_PIN_TEST);
        CU_ASSERT_EQUAL(res, LE_OK);

        fprintf(stderr, "\nle_sim_ChangePIN, res.%d (should be LE_OK=0)\n", res);
        fprintf(stderr, "\nTake off, then insert SIM card.%d, wait for +WIND:1 (approx. 2s) and then press enter \n", i);
        while ( getchar() != '\n' );

        // Unblock PIN
        while((initTries=le_sim_GetRemainingPINTries(simRef))>0)
        {
            res = le_sim_EnterPIN(simRef, FAIL_PIN_TEST);
        }

        if(initTries < 0)
        {
            fprintf(stderr, "\nle_sim_GetRemainingPINTries error, res.%d (should be >=0)\n", initTries);
        }

        res = le_sim_Unblock(simRef, (char*)&PUK_TEST[i-1][0], PIN_TOO_LONG_TEST);
        CU_ASSERT_EQUAL(res, LE_OVERFLOW);
        res = le_sim_Unblock(simRef, (char*)&PUK_TEST[i-1][0], PIN_TOO_SHORT_TEST);
        CU_ASSERT_EQUAL(res, LE_UNDERFLOW);
        res = le_sim_Unblock(simRef, PUK_BAD_LENGTH_TEST, NEW_PIN_TEST);
        CU_ASSERT_EQUAL(res, LE_OUT_OF_RANGE);
        res = le_sim_Unblock(simRef, FAIL_PUK_TEST, NEW_PIN_TEST);
        CU_ASSERT_EQUAL(res, LE_NOT_POSSIBLE);
        res = le_sim_Unblock(simRef, (char*)&PUK_TEST[i-1][0], (char*)&internalPin[i-1][0]);
        CU_ASSERT_EQUAL(res, LE_OK);

        fprintf(stderr, "\nle_sim_Unblock, res.%d  (should be LE_OK=0), press enter to continue \n", res);
        while ( getchar() != '\n' );

        // Unlock PIN
        res = le_sim_Unlock(simRef, PIN_TOO_LONG_TEST);
        CU_ASSERT_EQUAL(res, LE_OVERFLOW);
        res = le_sim_Unlock(simRef, PIN_TOO_SHORT_TEST);
        CU_ASSERT_EQUAL(res, LE_UNDERFLOW);
        res = le_sim_Unlock(simRef, FAIL_PIN_TEST);
        CU_ASSERT_EQUAL(res, LE_NOT_POSSIBLE);
        res = le_sim_Unlock(simRef, (char*)&internalPin[i-1][0]);
        CU_ASSERT_EQUAL(res, LE_OK);

        fprintf(stderr, "\nle_sim_Unlock, res.%d  (should be LE_OK=0), press enter to continue  \n", res);
        while ( getchar() != '\n' );

        // Re-lock the SIM card
        if (pinReq)
        {
            res = le_sim_Lock(simRef, (char*)&internalPin[i-1][0]);
            CU_ASSERT_EQUAL(res, LE_OK);
        }

        le_sim_Delete(simRef);
        i++;
    } while (i<=le_sim_CountSlots());

    // Test case for SIM card absent: executed only on first slot
    simRef = le_sim_Create(1);
    CU_ASSERT_PTR_NOT_NULL(simRef);

    fprintf(stderr, "Take off SIM card.1 and then press enter \n");
    while ( getchar() != '\n' );

    // Enter PIN
    initTries=le_sim_GetRemainingPINTries(simRef);
    CU_ASSERT_TRUE((initTries == LE_NOT_FOUND) || (initTries == LE_NOT_POSSIBLE));

    res = le_sim_EnterPIN(simRef, PIN_TEMP);
    CU_ASSERT_TRUE((res == LE_NOT_FOUND) || (res == LE_NOT_POSSIBLE));

    ready=le_sim_IsReady(simRef);
    CU_ASSERT_FALSE(ready);

    // Change PIN
    res = le_sim_ChangePIN(simRef, PIN_TEMP, NEW_PIN_TEST);
    CU_ASSERT_TRUE((res == LE_NOT_FOUND) || (res == LE_NOT_POSSIBLE));

    // Unblock PIN
    res = le_sim_Unblock(simRef, (char*)&PUK_TEST[0][0], PIN_TEMP);
    CU_ASSERT_TRUE((res == LE_NOT_FOUND) || (res == LE_NOT_POSSIBLE));

    // Unlock PIN
    res = le_sim_Unlock(simRef, PIN_TEMP);
    CU_ASSERT_TRUE((res == LE_NOT_FOUND) || (res == LE_NOT_POSSIBLE));

    le_sim_Delete(simRef);

    fprintf(stderr, "Insert SIM card.1, wait for +WIND:1 (approx. 2s) and then press enter \n");
    while ( getchar() != '\n' );
}



//--------------------------------------------------------------------------------------------------
/**
 * Test: SIM States.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sim_States()
{
    le_result_t                 res;
    le_sim_NewStateHandlerRef_t testHdlrRef;
    le_sim_States_t             state;
    le_sim_ObjRef_t             simRef;
    uint32_t                    i=1;

    do
    {
        fprintf(stderr, "Insert SIM card.%d, wait for +WIND:1 (approx. 2s) and then press enter \n", i);
        while ( getchar() != '\n' );

        simRef = le_sim_Create(i);
        CU_ASSERT_PTR_NOT_NULL(simRef);

        state = le_sim_GetState(simRef);
        CU_ASSERT_TRUE((state>=LE_SIM_INSERTED) && (state<=LE_SIM_BUSY));
        fprintf(stderr, "\nSIM Card.%d state:\n", i);
        displaySIMState(state, i);
        // Enter PIN
        if(state == LE_SIM_INSERTED)
        {
            res = le_sim_EnterPIN(simRef, (char*)&PIN_TEST[i-1][0]);
            CU_ASSERT_EQUAL(res, LE_OK);

            state = le_sim_GetState(simRef);
            CU_ASSERT_TRUE((state>=LE_SIM_INSERTED) && (state<=LE_SIM_BUSY));
        }

        le_sim_Delete(simRef);
        i++;
    } while (i<=le_sim_CountSlots());


    testHdlrRef=le_sim_AddNewStateHandler(TestSimStateHandler, NULL);
    CU_ASSERT_PTR_NOT_NULL(testHdlrRef);
}


