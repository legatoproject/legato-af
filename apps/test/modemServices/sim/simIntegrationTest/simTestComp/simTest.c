 /**
  * This module implements the le_sim's tests.
  *
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "main.h"

#define PIN_TEMP            "0123"
#define PIN_TEST1           "1111"
#define PIN_TEST2           "2222"
#define PIN_TOO_LONG_TEST   "123456789"
#define PIN_TOO_SHORT_TEST  "12"
#define PUK_TEST1           "11111111"
#define PUK_TEST2           "22222222"
#define PUK_BAD_LENGTH_TEST "12"
//! [Define]
#define NEW_PIN_TEST        "5678"
#define FAIL_PIN_TEST       "4321"
#define FAIL_PUK_TEST       "87654321"
//! [Define]
#define SIM_RSP_LEN         100

static le_sem_Ref_t    SimPowerCycleSemaphore;
static le_thread_Ref_t  SimPowerCycleThreadRef = NULL;
static le_sim_NewStateHandlerRef_t   SimPowerCycleHdlrRef = NULL;
static bool SimPowerCycleStarted = false;

//! [State handler]
//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SIM States Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSimStateHandler
(
    le_sim_Id_t     simId,       ///< [IN] SIM identifier
    le_sim_States_t simState,    ///< [IN] SIM state
    void*           contextPtr   ///< [IN] Context
)
{
    switch (simState)
    {
        case LE_SIM_INSERTED:
            LE_INFO("-TEST- New state LE_SIM_INSERTED for SIM card.%d", simId);
            break;
        case LE_SIM_ABSENT:
            LE_INFO("-TEST- New state LE_SIM_ABSENT for SIM card.%d", simId);
            break;
        case LE_SIM_READY:
            LE_INFO("-TEST- New state LE_SIM_READY for SIM card.%d", simId);
            if (SimPowerCycleStarted)
            {
                SimPowerCycleStarted = false;
                le_sem_Post(SimPowerCycleSemaphore);
            }
            break;
        case LE_SIM_BLOCKED:
            LE_INFO("-TEST- New state LE_SIM_BLOCKED for SIM card.%d", simId);
            break;
        case LE_SIM_BUSY:
            LE_INFO("-TEST- New state LE_SIM_BUSY for SIM card.%d", simId);
            break;
        case LE_SIM_POWER_DOWN:
            LE_INFO("-TEST- New state LE_SIM_POWER_DOWN for SIM card.%d", simId);
            if (SimPowerCycleStarted)
            {
                le_sem_Post(SimPowerCycleSemaphore);
            }
            break;
        case LE_SIM_STATE_UNKNOWN:
            LE_INFO("-TEST- New state LE_SIM_STATE_UNKNOWN for SIM card.%d", simId);
            break;
        default:
            LE_INFO("-TEST- New state %d for SIM card.%d", simState, simId);
            break;
    }
}
//! [State handler]

//! [Display]
//--------------------------------------------------------------------------------------------------
/**
 * This function display the SIM state.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DisplaySimState
(
    le_sim_States_t state,
    le_sim_Id_t simId
)
{
    char string[100];

    memset(string,0,100);

    switch(state)
    {
        case LE_SIM_INSERTED:
            sprintf(string, "\nSIM Card state LE_SIM_INSERTED for SIM card.%d \n", simId);
            break;
        case LE_SIM_ABSENT:
            sprintf(string, "\nSIM Card state LE_SIM_ABSENT for SIM card.%d \n", simId);
            break;
        case LE_SIM_READY:
            sprintf(string, "\nSIM Card state LE_SIM_READY for SIM card.%d \n", simId);
            break;
        case LE_SIM_BLOCKED:
            sprintf(string, "\nSIM Card state LE_SIM_BLOCKED for SIM card.%d \n", simId);
            break;
        case LE_SIM_BUSY:
            sprintf(string, "\nSIM Card state LE_SIM_BUSY for SIM card.%d \n", simId);
            break;
        case LE_SIM_POWER_DOWN:
            sprintf(string, "\nSIM Card state LE_SIM_POWER_DOWN for SIM card.%d \n", simId);
            break;
        case LE_SIM_STATE_UNKNOWN:
            sprintf(string, "\nSIM Card state LE_SIM_STATE_UNKNOWN for SIM card.%d \n", simId);
            break;
        default:
            sprintf(string, "\nSIM Card state %d for SIM card.%d \n", state, simId);
            break;
    }

    Print(string);
}
//! [Display]

void StateHandlerFunc
(
    le_sim_Id_t     simId,
    le_sim_States_t simState,
    void*           contextPtr
)
{
    LE_INFO("StateHandlerFunc simId %d, state %d", simId, simState);
}

//--------------------------------------------------------------------------------------------------
/**
 * Print APDU.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintApdu
(
    uint8_t* rsp,
    uint32_t rspLen
)
{
    LE_INFO("rspLen %d" , rspLen);
    char string[5*rspLen+1];
    uint32_t i = 0;
    memset(string,0,5*rspLen+1);

    while (i < rspLen)
    {
        sprintf(string+strlen(string),"0x%02X ", rsp[i] );
        i++;
    }

    LE_INFO("APDU response: %s", string );
}


//--------------------------------------------------------------------------------------------------
//                                       Test Functions
//--------------------------------------------------------------------------------------------------

//! [Identification]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Creation & information retrieving.
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_Create
(
    le_sim_Id_t simId,
    const char* pinPtr
)
{
    bool            presence = false;
    char            iccid[LE_SIM_ICCID_BYTES] = {0};
    char            imsi[LE_SIM_IMSI_BYTES] = {0};
    char            eid[LE_SIM_EID_BYTES] = {0};

    // Enter PIN code
    LE_ASSERT_OK(le_sim_EnterPIN(simId, pinPtr));

    // Get ICCID
    LE_ASSERT_OK(le_sim_GetICCID(simId, iccid, sizeof(iccid)));
    Print( iccid );

    // Get EID
    LE_ASSERT_OK(le_sim_GetEID(simId, eid, sizeof(eid)));
    Print( eid );

    // Get IMSI
    LE_ASSERT_OK(le_sim_GetIMSI(simId, imsi, sizeof(imsi)));
    Print( imsi );

    // Check if SIM present
    presence = le_sim_IsPresent(simId);
    LE_ASSERT(presence);
}
//! [Identification]

//--------------------------------------------------------------------------------------------------
/**
 * Test: SIM lock/unlock.
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_Lock
(
    le_sim_Id_t simId,
    const char* pinPtr
)
{
    le_sim_States_t state;
    bool doLock = false;
    uint8_t loop = 0;
    le_result_t     res;

    // Get SIM state
    state = le_sim_GetState(simId);
    DisplaySimState(state, simId);
    LE_ASSERT((state == LE_SIM_READY)||(state == LE_SIM_INSERTED));

    if (LE_SIM_READY == state)
    {
        doLock = true;
    }
    else if (LE_SIM_INSERTED == state)
    {
        // Enter PIN code
        res = le_sim_EnterPIN(simId, pinPtr);
        LE_ASSERT(res == LE_OK);
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
            res = le_sim_Lock(simId, FAIL_PIN_TEST);
            LE_ASSERT(res == LE_FAULT);

            // Lock PIN using the correct PIN code
            res = le_sim_Lock(simId, pinPtr);
            LE_ASSERT(res == LE_OK);

            // Enter PIN code
            res = le_sim_EnterPIN(simId, pinPtr);
            LE_ASSERT(res == LE_OK);
            doLock = false;
        }
        else
        {
            LE_INFO("unlock the SIM");

            // Unlock the SIM using a wrong PIN code (error code expected)
            res = le_sim_Unlock(simId, FAIL_PIN_TEST);
            LE_ASSERT(res == LE_FAULT);

            // Lock PIN using the correct PIN code
            res = le_sim_Unlock(simId, pinPtr);
            LE_ASSERT(res == LE_OK);
            doLock = true;
        }

        loop++;
    }
}

//! [Authentication]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Authentication (pin/puk).
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_Authentication
(
    le_sim_Id_t simId,
    const char* pinPtr,
    const char* pukPtr
)
{
    le_result_t     res;
    bool            ready = false;
    int32_t         remainingPinTries = 0;
    int32_t         tries = 0;
    uint32_t        remainingPukTries = 0;
    uint32_t        pukTries = 0;
    char            string[100];

    memset(string, 0, 100);

    // Get the remaining PIN entries
    remainingPinTries = le_sim_GetRemainingPINTries(simId);

    // Enter PIN code
    res = le_sim_EnterPIN(simId, FAIL_PIN_TEST);
    LE_ASSERT(res == LE_FAULT);

    // Get the remaining PIN entries
    tries = le_sim_GetRemainingPINTries(simId);
    LE_ASSERT((remainingPinTries-tries) == 1);

    // Check that the SIM is not ready
    ready = le_sim_IsReady(simId);
    LE_ASSERT(!ready);

    // Enter PIN code
    res = le_sim_EnterPIN(simId, pinPtr);
    LE_ASSERT(res == LE_OK);

    // Check that the SIM is ready
    ready = le_sim_IsReady(simId);
    LE_ASSERT(ready);

    // Change PIN
    res = le_sim_ChangePIN(simId, FAIL_PIN_TEST, NEW_PIN_TEST);
    LE_ASSERT(res == LE_FAULT);

    // Change the PIN code
    res = le_sim_ChangePIN(simId, pinPtr, NEW_PIN_TEST);
    LE_ASSERT(res == LE_OK);

    // block the SIM:
    // while remaining PIN entries not null, enter a wrong PIN code
    while((remainingPinTries = le_sim_GetRemainingPINTries(simId)) > 0)
    {
        // Enter PIN code
        res = le_sim_EnterPIN(simId, FAIL_PIN_TEST);
    }

    if(remainingPinTries < 0)
    {
        sprintf(string, "\nle_sim_GetRemainingPINTries error, res.%d (should be >=0)\n",
                remainingPinTries);
        Print(string);
    }

    // Get the remaining PUK entries
    LE_ASSERT_OK(le_sim_GetRemainingPUKTries(simId, &remainingPukTries));

    // Unblock the SIM using a wrong PUK code (error expected)
    res = le_sim_Unblock(simId, FAIL_PUK_TEST, NEW_PIN_TEST);
    LE_ASSERT(res == LE_FAULT);

    // Get the remaining PUK entries
    LE_ASSERT_OK(le_sim_GetRemainingPUKTries(simId, &pukTries));
    LE_ASSERT(1 == (remainingPukTries-pukTries));

    // Unblock the SIM using the correct PUK code
    res = le_sim_Unblock(simId, pukPtr, NEW_PIN_TEST);
    LE_ASSERT(LE_OK == res);

    // Get the remaining PUK entries
    LE_ASSERT_OK(le_sim_GetRemainingPUKTries(simId, &pukTries));
    LE_ASSERT((0 == remainingPukTries-pukTries));

    Print("End simTest_Authentication");
}
//! [Authentication]

//--------------------------------------------------------------------------------------------------
/**
 * Test: SIM absent.
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_SimAbsent
(
    le_sim_Id_t simId
)
{
    int32_t         remainingPinTries = 0;
    uint32_t        remainingPukTries = 0;
    le_result_t     res;
    bool            ready = false;

    // Get the remaining PIN entries (error expected as no SIM)
    remainingPinTries = le_sim_GetRemainingPINTries(simId);
    LE_ASSERT((remainingPinTries == LE_NOT_FOUND) || (remainingPinTries == LE_FAULT));

    // Get the remaining PUK entries (error expected as no SIM)
    res = le_sim_GetRemainingPUKTries(simId, &remainingPukTries);
    LE_ASSERT((res == LE_NOT_FOUND) || (res == LE_FAULT));

    // Enter PIN code (error expected as no SIM)
    res = le_sim_EnterPIN(simId, PIN_TEMP);
    LE_ASSERT((res == LE_NOT_FOUND) || (res == LE_FAULT));

    // Check that the SIM is not ready
    ready = le_sim_IsReady(simId);
    LE_ASSERT(!ready);

    // Change PIN (error expected as no SIM)
    res = le_sim_ChangePIN(simId, PIN_TEMP, NEW_PIN_TEST);
    LE_ASSERT((res == LE_NOT_FOUND) || (res == LE_FAULT));

    // Unblock PIN  (error expected as no SIM)
    res = le_sim_Unblock(simId, (char*)PUK_TEST1, PIN_TEMP);
    LE_ASSERT((res == LE_NOT_FOUND) || (res == LE_FAULT));

    // Unlock PIN  (error expected as no SIM)
    res = le_sim_Unlock(simId, PIN_TEMP);
    LE_ASSERT((res == LE_NOT_FOUND) || (res == LE_FAULT));
}

//! [Select]
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
    // Select the embedded SIM
    LE_ASSERT_OK(le_sim_SelectCard(LE_SIM_EMBEDDED));

    // Get the selected card
    le_sim_Id_t simId = le_sim_GetSelectedCard();
    LE_ASSERT(LE_SIM_EMBEDDED == simId);

    // Select the LE_SIM_EXTERNAL_SLOT_1 SIM
    LE_ASSERT_OK(le_sim_SelectCard(LE_SIM_EXTERNAL_SLOT_1));

    // Get the selected card
    simId = le_sim_GetSelectedCard();
    LE_ASSERT(LE_SIM_EXTERNAL_SLOT_1 == simId);

    // Check if SIM present
    if (!le_sim_IsPresent(LE_SIM_EMBEDDED))
    {
        LE_INFO("SIM not present");
    }

    // Get the selected card by le_sim_GetSelectedCard()
    // Notice that the selected card received is the one used by the
    // last Legato API and not the one set by le_sim_SelectCard().
    simId = le_sim_GetSelectedCard();
    LE_ASSERT(LE_SIM_EMBEDDED == simId);

    // Check SIM ready
    if (!le_sim_IsReady(LE_SIM_EXTERNAL_SLOT_1))
    {
        LE_INFO("SIM not ready");
    }

    // Get the selected card by le_sim_GetSelectedCard()
    // Notice that the selected card received is the one used by the
    // last Legato API and not the one set by le_sim_SelectCard().
    simId = le_sim_GetSelectedCard();
    LE_ASSERT(LE_SIM_EXTERNAL_SLOT_1 == simId);


}
//! [Select]

//! [State]
//--------------------------------------------------------------------------------------------------
/**
 * Test: SIM State.
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_State
(
    le_sim_Id_t simId,
    const char* pinPtr
)
{
    le_sim_States_t             state;
    le_sim_NewStateHandlerRef_t testHdlrRef;
    char                        string[100];

    memset(string, 0, 100);

    // Add the state handler
    testHdlrRef = le_sim_AddNewStateHandler(TestSimStateHandler, NULL);
    LE_ASSERT(NULL != testHdlrRef);

    // Get SIM state
    state = le_sim_GetState(simId);

    LE_INFO("test: state %d", state);

    LE_ASSERT((state >= LE_SIM_INSERTED) && (state <= LE_SIM_BUSY));
    sprintf(string, "\nSIM Card.%d state:\n", simId);
    Print(string);

    DisplaySimState(state, simId);

    if (LE_SIM_INSERTED == state)
    {
        // Enter PIN code
        LE_ASSERT_OK(le_sim_EnterPIN(simId, pinPtr));

        // Get SIM state
        state = le_sim_GetState(simId);
        LE_ASSERT((state>=LE_SIM_INSERTED) && (state<=LE_SIM_BUSY));
    }
}
//! [State]

//--------------------------------------------------------------------------------------------------
/**
 * Test: SIM Get ICCID.
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_SimGetIccid
(
    le_sim_Id_t simId
)
{
    le_result_t     res;
    char            iccid[LE_SIM_ICCID_BYTES];
    char            string[100];

    memset(iccid, 0, LE_SIM_ICCID_BYTES);

    LE_INFO("SimId %d", simId);

    // Get SIM ICCID
    res = le_sim_GetICCID(simId, iccid, sizeof(iccid));

    LE_ASSERT(res == LE_OK);

    sprintf(string, "\nSIM Card ICCID: '%s'\n", iccid);
    Print(string);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: SIM Get EID.
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_SimGetEid
(
    le_sim_Id_t simId
)
{
    le_result_t     res;
    char            eid[LE_SIM_EID_BYTES];
    // The string size table is enought to hold "\nSIM Card EID: LE_SIM_EID_BYTES \n"
    char            string[LE_SIM_EID_BYTES+40];

    memset(eid, 0, LE_SIM_EID_BYTES);

    LE_INFO("SimId %d", simId);

    // Get SIM ICCID
    res = le_sim_GetEID(simId, eid, sizeof(eid));

    LE_ASSERT(res == LE_OK);

    snprintf(string, sizeof(string),"\nSIM Card EID: '%s'\n", eid);
    Print(string);
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread for test LE_SIM_POWER_DOWN  indication.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* SimPowerCycleIndThread
(
    void* context   ///< Context
)
{
    le_sim_ConnectService();

    SimPowerCycleHdlrRef = le_sim_AddNewStateHandler(TestSimStateHandler, NULL);

    LE_ASSERT(SimPowerCycleHdlrRef);

    //First sem post and  the "simPowerCycleStarted" flag indicate this thread is running
    le_sem_Post(SimPowerCycleSemaphore);
    SimPowerCycleStarted = true;

    LE_INFO("SimPowerCycleIndThread started ...");

    le_event_RunLoop();

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Powers up or down the current SIM card.
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_SimPowerUpDown
(
    void
)
{
    le_sim_States_t  state;
    le_sim_Id_t simId;
    le_result_t res;
    le_clk_Time_t timeOut;

    //set timeout seconds for waiting for asynchronous power down event.
    timeOut.sec = 5;
    timeOut.usec = 0;

    SimPowerCycleSemaphore = le_sem_Create("HandlerSimPowerCycle", 0);

    SimPowerCycleThreadRef= le_thread_Create("ThreadSimPowerCycle", SimPowerCycleIndThread, NULL);

    le_thread_Start(SimPowerCycleThreadRef);

    // get blocked here until our event handler is registered and powerCycle thread is running
    res = le_sem_WaitWithTimeOut(SimPowerCycleSemaphore, timeOut);
    LE_ASSERT_OK(res);

    // Power down cases
    simId = le_sim_GetSelectedCard();
    state = le_sim_GetState(simId);
    LE_INFO("test: SIM state %d", state);
    LE_ASSERT(LE_SIM_READY == state);
    LE_ASSERT_OK(le_sim_SetPower(simId, LE_OFF));

    // Wait for complete asynchronous event of powered down (LE_SIM_POWER_DOWN)
    res = le_sem_WaitWithTimeOut(SimPowerCycleSemaphore, timeOut);
    LE_ASSERT_OK(res);
    LE_INFO("Powers Down current SIM: success");

    // Power up cases
    LE_ASSERT_OK(le_sim_SetPower(simId, LE_ON));
    // Wait for complete asynchronous event of powered up  (LE_SIM_READY)
    res = le_sem_WaitWithTimeOut(SimPowerCycleSemaphore, timeOut);
    LE_ASSERT_OK(res);
    LE_INFO("Powers On current SIM: success");

    // Remove the handler
    le_sim_RemoveNewStateHandler(SimPowerCycleHdlrRef);

    // cancel the power cycle test thread
    le_thread_Cancel(SimPowerCycleThreadRef);
}

//! [Apdu]
//--------------------------------------------------------------------------------------------------
/**
 * Test: SIM access.
 *
 */
//--------------------------------------------------------------------------------------------------
void simTest_SimAccess
(
    le_sim_Id_t simId
)
{
    //========================================
    // 1. Read IMSI using le_sim_SendApdu API
    //========================================

    uint8_t selectDfAdfApdu[] = {0x00, 0xA4, 0x00, 0x0C, 0x02, 0x7F, 0xFF};
    uint8_t selectApdu[] = {0x00, 0xA4, 0x00, 0x0C, 0x02, 0x6F, 0x07};
    uint8_t readApdu[] = {0x00, 0xB0, 0x00, 0x00, 0x09};
    uint8_t rspImsi[SIM_RSP_LEN];
    size_t rspImsiLen = SIM_RSP_LEN;

    // Select ADF Dedicated File (DF_ADF)
    LE_ASSERT_OK(le_sim_SendApdu(simId,
                                 selectDfAdfApdu,
                                 sizeof(selectDfAdfApdu),
                                 rspImsi,
                                 &rspImsiLen));
    PrintApdu(rspImsi, rspImsiLen);

    // Select the EF(IMSI)
    rspImsiLen = SIM_RSP_LEN;
    LE_ASSERT_OK(le_sim_SendApdu(simId,
                                 selectApdu,
                                 sizeof(selectApdu),
                                 rspImsi,
                                 &rspImsiLen));
    PrintApdu(rspImsi, rspImsiLen);

    // Read the EF(IMSI)
    rspImsiLen = SIM_RSP_LEN;
    LE_ASSERT_OK(le_sim_SendApdu(simId,
                                 readApdu,
                                 sizeof(readApdu),
                                 rspImsi,
                                 &rspImsiLen));
    PrintApdu(rspImsi, rspImsiLen);

    //=====================================================================================
    // 2. Read IMSI using le_sim_SendSimCommand API, and check value get by le_sim_SendApdu
    //======================================================================================

    size_t rspImsiLen2 = SIM_RSP_LEN;
    uint8_t rspImsi2[SIM_RSP_LEN];
    uint8_t swi1, swi2;
    char dfGsmPath[]="3F007FFF";

    // Read EF(IMSI) using the le_sim_SendSimCommand API.
    le_result_t res = le_sim_SendCommand(simId,
                                         LE_SIM_READ_BINARY,
                                         "6F07",
                                         0,
                                         0,
                                         0,
                                         NULL,
                                         0,
                                         dfGsmPath,
                                         &swi1,
                                         &swi2,
                                         rspImsi2,
                                         &rspImsiLen2);

    if (LE_UNSUPPORTED == res)
    {
        LE_WARN("le_sim_SendCommand() API not supported by the platform");
        return;
    }

    if (res != LE_OK)
    {
        strcpy(dfGsmPath, "3F007F20");

        // Check backward compatibility
        res = le_sim_SendCommand(simId,
                                 LE_SIM_READ_BINARY,
                                 "6F07",
                                 0,
                                 0,
                                 0,
                                 NULL,
                                 0,
                                 dfGsmPath,
                                 &swi1,
                                 &swi2,
                                 rspImsi2,
                                 &rspImsiLen2);
    }

    LE_ASSERT(res == LE_OK);

    LE_INFO("swi1=0x%02X, swi2=0x%02X", swi1, swi2);
    PrintApdu(rspImsi2, rspImsiLen2);

    // Check both IMSI results
    LE_ASSERT(0 == memcmp(rspImsi, rspImsi2, rspImsiLen2));

    size_t rspLen = SIM_RSP_LEN;
    uint8_t rsp[rspLen];

    //==================================================================================
    // 3. Check read and write record elementary file
    // Write the 5th entry in EF(ADN): equivalent to AT+CPBW=5,"01290917",129,"Jacky"
    // Then, read the written data and check
    //==================================================================================

    // Write EF(ADN) using the le_sim_SendSimCommand API.
    uint8_t dataAdn[] = {0x4A, 0x61, 0x63, 0x6B, 0x79, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                         0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x81, 0x10, 0x92, 0x90, 0x71,
                         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                         0xFF, 0xFF, 0xFF, 0xFF};

    LE_ASSERT_OK(le_sim_SendCommand(simId,
                                    LE_SIM_UPDATE_RECORD,
                                    "6F3A",
                                    5,
                                    0,
                                    0,
                                    dataAdn,
                                    sizeof(dataAdn),
                                    "3F007F10",
                                    &swi1,
                                    &swi2,
                                    rsp,
                                    &rspLen));

    LE_INFO("swi1=0x%02X, swi2=0x%02X", swi1, swi2);

    // Read EF(ADN) using the le_sim_SendSimCommand API.
    rspLen = SIM_RSP_LEN;
    LE_ASSERT_OK(le_sim_SendCommand(simId,
                                    LE_SIM_READ_RECORD,
                                    "6F3A",
                                    5,
                                    0,
                                    0,
                                    NULL,
                                    0,
                                    "3F007F10",
                                    &swi1,
                                    &swi2,
                                    rsp,
                                    &rspLen));

    LE_INFO("swi1=0x%02X, swi2=0x%02X", swi1, swi2);
    PrintApdu(rsp, rspLen);

    LE_ASSERT(rspLen == sizeof(dataAdn));
    LE_ASSERT(0 == memcmp(rsp, dataAdn, rspLen));

    //==================================================================================
    // 4. Check read and write transparent elementary file
    // - Read language indication file
    // - Erase first entry of the file
    // - Check that it is really erased (by reading again)
    // - Re-write the initial value
    // - Check that the initial value is correct (read again)
    //==================================================================================

    // Read binary EF(6F05) Language indication
    size_t rspLenLi = SIM_RSP_LEN;
    uint8_t rspLi[rspLenLi];

    LE_ASSERT_OK(le_sim_SendCommand(simId,
                                    LE_SIM_READ_BINARY,
                                    "6F05",
                                    0,
                                    0,
                                    0,
                                    NULL,
                                    0,
                                    dfGsmPath,
                                    &swi1,
                                    &swi2,
                                    rspLi,
                                    &rspLenLi));

    LE_INFO("swi1=0x%02X, swi2=0x%02X", swi1, swi2);
    PrintApdu(rspLi, rspLenLi);

    uint8_t dataLi[] = {0xFF, 0xFF};
    // Erase first Language entry
    rspLen = 0;
    LE_ASSERT_OK(le_sim_SendCommand(simId,
                                    LE_SIM_UPDATE_BINARY,
                                    "6F05",
                                    0,
                                    0,
                                    0,
                                    dataLi,
                                    sizeof(dataLi),
                                    dfGsmPath,
                                    &swi1,
                                    &swi2,
                                    rsp,
                                    &rspLen));

    LE_INFO("swi1=0x%02X, swi2=0x%02X", swi1, swi2);

    // Read again...
    rspLen = SIM_RSP_LEN;
    LE_ASSERT_OK(le_sim_SendCommand(simId,
                                    LE_SIM_READ_BINARY,
                                    "6F05",
                                    0,
                                    0,
                                    0,
                                    NULL,
                                    0,
                                    dfGsmPath,
                                    &swi1,
                                    &swi2,
                                    rsp,
                                    &rspLen));

    LE_INFO("swi1=0x%02X, swi2=0x%02X", swi1, swi2);
    PrintApdu(rsp, rspLen);

    // Check it is correctly erased
    LE_ASSERT(0 == memcmp(rsp, dataLi, sizeof(dataLi)));

    // Re-write initial values
    rspLen = 0;
    LE_ASSERT_OK(le_sim_SendCommand(simId,
                                    LE_SIM_UPDATE_BINARY,
                                    "6F05",
                                    0,
                                    0,
                                    0,
                                    rspLi,
                                    rspLenLi,
                                    dfGsmPath,
                                    &swi1,
                                    &swi2,
                                    rsp,
                                    &rspLen));

    LE_INFO("swi1=0x%02X, swi2=0x%02X", swi1, swi2);

    // And read again...
    rspLen = SIM_RSP_LEN;
    LE_ASSERT_OK(le_sim_SendCommand(simId,
                                    LE_SIM_READ_BINARY,
                                    "6F05",
                                    0,
                                    0,
                                    0,
                                    NULL,
                                    0,
                                    dfGsmPath,
                                    &swi1,
                                    &swi2,
                                    rsp,
                                    &rspLen));

    LE_INFO("swi1=0x%02X, swi2=0x%02X", swi1, swi2);
    PrintApdu(rsp, rspLen);

    // Check it is correctly erased
    LE_ASSERT(0 == memcmp(rsp, rspLi, sizeof(rspLenLi)));

    //=====================================================================================
    // 5. Read IMSI using a dedicated logical channel
    // Note that a SIM card supporting logical channels is necessary for this test.
    //======================================================================================

    uint8_t channel = 0;

    // Open a logical channel
    LE_ASSERT_OK(le_sim_OpenLogicalChannel(&channel));
    LE_ASSERT(channel);

    // Select ADF Dedicated File (DF_ADF)
    selectDfAdfApdu[0] = channel;
    LE_ASSERT_OK(le_sim_SendApduOnChannel(simId,
                                          channel,
                                          selectDfAdfApdu,
                                          sizeof(selectDfAdfApdu),
                                          rspImsi,
                                          &rspImsiLen));
    PrintApdu(rspImsi, rspImsiLen);

    // Select the EF(IMSI)
    rspImsiLen = SIM_RSP_LEN;
    selectApdu[0] = channel;
    LE_ASSERT_OK(le_sim_SendApduOnChannel(simId,
                                          channel,
                                          selectApdu,
                                          sizeof(selectApdu),
                                          rspImsi,
                                          &rspImsiLen));
    PrintApdu(rspImsi, rspImsiLen);

    // Read the EF(IMSI)
    rspImsiLen = SIM_RSP_LEN;
    readApdu[0] = channel;
    LE_ASSERT_OK(le_sim_SendApduOnChannel(simId,
                                          channel,
                                          readApdu,
                                          sizeof(readApdu),
                                          rspImsi,
                                          &rspImsiLen));
    PrintApdu(rspImsi, rspImsiLen);

    // Close the logical channel
    LE_ASSERT_OK(le_sim_CloseLogicalChannel(channel));

    LE_INFO("SIM access test OK");
}
//! [Apdu]
