 /**
  * This module implements the le_mrc's unit tests.
  *
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

/*
 * Module need to be registered on the SIM home PLMN.
 *
 * Instruction to execute this test
 * 1) install application test.
 * 2) Start log trace 'logread -f | grep 'INFO'
 * 3) Start application 'app start mrcTest'
 * 4) check trace for the following INFO  trace:
 *     "======== Test MRC Modem Services implementation Test SUCCESS ========"
 */

#include "legato.h"
#include <interfaces.h>


/*
 * Flag set to 1 to perform the Radio Power test On AR7 platform
 */
#if AR7_DETECTED
#define TEST_MRC_POWER 1
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Semaphore to synchronize threads.
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t    ThreadSemaphore;

//--------------------------------------------------------------------------------------------------
/**
 * Registration Thread reference.
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t  RegistrationThreadRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Home PLMN references
 */
//--------------------------------------------------------------------------------------------------
static char mccHomeStr[LE_MRC_MCC_BYTES] = {0};
static char mncHomeStr[LE_MRC_MCC_BYTES] = {0};


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for RAT change Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestRatHandler
(
    le_mrc_Rat_t rat,
    void*        contextPtr
)
{
    LE_INFO("New RAT: %d", rat);

    if (rat == LE_MRC_RAT_CDMA)
    {
        LE_INFO("Check RatHandler passed, RAT is LE_MRC_RAT_CDMA.");
    }
    else if (rat == LE_MRC_RAT_GSM)
    {
        LE_INFO("Check RatHandler passed, RAT is LE_MRC_RAT_GSM.");
    }
    else if (rat == LE_MRC_RAT_UMTS)
    {
        LE_INFO("Check RatHandler passed, RAT is LE_MRC_RAT_UMTS.");
    }
    else if (rat == LE_MRC_RAT_LTE)
    {
        LE_INFO("Check RatHandler passed, RAT is LE_MRC_RAT_LTE.");
    }
    else
    {
        LE_INFO("Check RatHandler failed, bad RAT.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for PS change notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestPSHandler
(
    le_mrc_ServiceState_t psState,
    void*        contextPtr
)
{
    LE_INFO("New PS state: %d", psState);

    if (psState == LE_MRC_DETACHED)
    {
        LE_INFO("Check PSHandler passed, PS is LE_MRC_DETACHED.");
    }
    else if (psState == LE_MRC_ATTACHED)
    {
        LE_INFO("Check PSHandler passed, PS is LE_MRC_ATTACHED.");
    }
    else
    {
        LE_ERROR("Check PSHandler failed, bad PS state %d", psState);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for CS change notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestCSHandler
(
    le_mrc_ServiceState_t csState,
    void*        contextPtr
)
{
    LE_INFO("New CS state: %d", csState);

    if (csState == LE_MRC_DETACHED)
    {
        LE_INFO("Check CSHandler passed, CS is LE_MRC_DETACHED.");
    }
    else if (csState == LE_MRC_ATTACHED)
    {
        LE_INFO("Check CSHandler passed, CS is LE_MRC_ATTACHED.");
    }
    else
    {
        LE_ERROR("Check CSHandler failed, bad CS state %d", csState);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Network Registration Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestNetRegHandler
(
    le_mrc_NetRegState_t state,
    void*                contextPtr
)
{
    LE_INFO("New Network Registration state: %d", state);

    if (state == LE_MRC_REG_NONE)
    {
        LE_INFO("Check NetRegHandler passed, state is LE_MRC_REG_NONE.");
    }
    else if (state == LE_MRC_REG_HOME)
    {
        LE_INFO("Check NetRegHandler passed, state is LE_MRC_REG_HOME.");
    }
    else if (state == LE_MRC_REG_SEARCHING)
    {
        LE_INFO("Check NetRegHandler passed, state is LE_MRC_REG_SEARCHING.");
    }
    else if (state == LE_MRC_REG_DENIED)
    {
        LE_INFO("Check NetRegHandler passed, state is LE_MRC_REG_DENIED.");
    }
    else if (state == LE_MRC_REG_ROAMING)
    {
        LE_INFO("Check NetRegHandler passed, state is LE_MRC_REG_ROAMING.");
    }
    else if (state == LE_MRC_REG_UNKNOWN)
    {
        LE_INFO("Check NetRegHandler passed, state is LE_MRC_REG_UNKNOWN.");
    }
    else
    {
        LE_INFO("Check NetRegHandler failed, bad Network Registration state.");
    }
}



//--------------------------------------------------------------------------------------------------
//                                       Test Functions
//--------------------------------------------------------------------------------------------------

#if TEST_MRC_POWER
//! [Radio Power]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Radio Power Management.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_Power()
{
    le_result_t   res;
    le_onoff_t    onoff;

    res = le_mrc_SetRadioPower(LE_OFF);
    if (res != LE_OK)
    {
        le_mrc_SetRadioPower(LE_ON);
    }
    LE_ASSERT(res == LE_OK);

    sleep(5);

    res = le_mrc_GetRadioPower(&onoff);
    if ((res != LE_OK) || (onoff == LE_OFF))
    {
        le_mrc_SetRadioPower(LE_ON);
    }
    LE_ASSERT(res == LE_OK);
    LE_ASSERT(onoff == LE_OFF);

    res = le_mrc_SetRadioPower(LE_ON);
    LE_ASSERT(res == LE_OK);

    sleep(5);

    res = le_mrc_GetRadioPower(&onoff);
    LE_ASSERT(res == LE_OK);
    LE_ASSERT(onoff == LE_ON);

    sleep(5);
}
//! [Radio Power]
#endif

//! [RAT in Use]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Radio Access Technology.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_GetRat()
{
    le_result_t   res;
    le_mrc_Rat_t  rat;

    res = le_mrc_GetRadioAccessTechInUse(&rat);
    LE_ASSERT(res == LE_OK);
    if (res == LE_OK)
    {
        LE_ASSERT((rat>=LE_MRC_RAT_UNKNOWN) && (rat<=LE_MRC_RAT_LTE));
    }
    LE_INFO("le_mrc_GetRadioAccessTechInUse return rat 0x%02X",rat);
}
//! [RAT in Use]

//! [Service State]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Packet Switched state
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_GetPSState
(
    void
)
{
    le_mrc_ServiceState_t  psState;

    LE_ASSERT_OK(le_mrc_GetPacketSwitchedState(&psState));
    switch(psState)
    {
        case LE_MRC_ATTACHED:
            LE_INFO("le_mrc_GetPacketSwitchedState returns LE_MRC_ATTACHED");
            break;
        case LE_MRC_DETACHED:
            LE_INFO("le_mrc_GetPacketSwitchedState returns LE_MRC_DETACHED");
            break;
        default:
            LE_ERROR("le_mrc_GetPacketSwitchedState returns an unknown PS state");
            break;
    }

}
//! [Service State]

//! [Service State]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Circuit Switched state
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_GetCSState
(
    void
)
{
    le_mrc_ServiceState_t  csState;

    LE_ASSERT_OK(le_mrc_GetCircuitSwitchedState(&csState));
    switch(csState)
    {
        case LE_MRC_ATTACHED:
            LE_INFO("le_mrc_GetCircuitSwitchedState returns LE_MRC_ATTACHED");
            break;
        case LE_MRC_DETACHED:
            LE_INFO("le_mrc_GetCircuitSwitchedState returns LE_MRC_DETACHED");
            break;
        default:
            LE_ERROR("le_mrc_GetCircuitSwitchedState returns an unknown CS state");
            break;
    }

}
//! [Service State]

//--------------------------------------------------------------------------------------------------
/**
 * Test: Network Registration notification handling.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_NetRegHdlr()
{
    le_mrc_NetRegStateEventHandlerRef_t testHdlrRef;

    testHdlrRef = le_mrc_AddNetRegStateEventHandler(TestNetRegHandler, NULL);
    LE_ASSERT(testHdlrRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: RAT change handling.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_RatHdlr()
{
    le_mrc_RatChangeHandlerRef_t testHdlrRef;

    testHdlrRef = le_mrc_AddRatChangeHandler(TestRatHandler, NULL);
    LE_ASSERT(testHdlrRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: CS change handling.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_CSHdlr
(
    void
)
{
    le_mrc_CircuitSwitchedChangeHandlerRef_t testCSHdlrRef;

    testCSHdlrRef = le_mrc_AddCircuitSwitchedChangeHandler(TestCSHandler, NULL);
    LE_ASSERT(testCSHdlrRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: PS change handling.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_PSHdlr
(
    void
)
{
    le_mrc_PacketSwitchedChangeHandlerRef_t testPSHdlrRef;

    testPSHdlrRef = le_mrc_AddPacketSwitchedChangeHandler(TestPSHandler, NULL);
    LE_ASSERT(testPSHdlrRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Get the Current PLMN network.
 * le_mrc_GetCurrentNetworkMccMnc() API test
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_GetCurrentNetworkMccMnc()
{
    le_result_t res;
    int i;
    char mccRef[LE_MRC_MCC_BYTES] = {0};
    char mncRef[LE_MRC_MNC_BYTES] = {0};
    char mcc[LE_MRC_MCC_BYTES] = {0};
    char mnc[LE_MRC_MNC_BYTES] = {0};

    /* Test mccStrNumElements limit */
    res = le_mrc_GetCurrentNetworkMccMnc(mcc, LE_MRC_MCC_BYTES-1, mnc, LE_MRC_MNC_BYTES);
    LE_ASSERT(res == LE_FAULT);

    /* Test mncStrNumElements limit */
    res = le_mrc_GetCurrentNetworkMccMnc(mcc, LE_MRC_MCC_BYTES, mnc, LE_MRC_MNC_BYTES-1);
    LE_ASSERT(res == LE_FAULT);

    res = le_mrc_GetCurrentNetworkMccMnc(mccRef, LE_MRC_MCC_BYTES, mncRef, LE_MRC_MNC_BYTES);
    LE_ASSERT(res == LE_OK);
    LE_INFO("Plmn MCC.%s MNC.%s",mccRef,mncRef);

    for (i=0; i<10; i++)
    {
        res = le_mrc_GetCurrentNetworkMccMnc(mcc, LE_MRC_MCC_BYTES, mnc, LE_MRC_MNC_BYTES);
        LE_ASSERT(res == LE_OK);
        LE_ASSERT(strcmp(mnc, mncRef) == 0);
        LE_ASSERT(strcmp(mcc, mccRef) == 0);
        LE_INFO("Plmn MCC.%s MNC.%s",mcc,mnc);
    }
}

//! [Get Network]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Get the Current network name.
 * le_mrc_GetCurrentNetworkName() API test
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_GetCurrentNetworkName()
{

    le_result_t res;
    char nameStr[100] = {0};

    res = le_mrc_GetCurrentNetworkName(nameStr, 1);
    LE_ASSERT(res == LE_OVERFLOW);

    res = le_mrc_GetCurrentNetworkName(nameStr, 100);;
    LE_ASSERT(res == LE_OK);

    LE_INFO("Plmn name.%s",nameStr);
}
//! [Get Network]

//! [Register]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Register mode.
 * This test doesn't work in roaming.!!
 *
 * le_mrc_SetAutomaticRegisterMode() API test
 * le_mrc_SetManualRegisterMode() API test
 * le_mrc_GetRegisterMode() API test
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_RegisterMode()
{
    le_result_t res;
    int cmpRes;
    char mccStr[LE_MRC_MNC_BYTES] = {0};
    char mncStr[LE_MRC_MNC_BYTES] = {0};
    bool isManualOrigin, isManual;

    // Get the home PLMN to compare results.
    res = le_sim_GetHomeNetworkMccMnc(LE_SIM_EXTERNAL_SLOT_1, mccHomeStr, LE_MRC_MCC_BYTES,
                                      mncHomeStr, LE_MRC_MNC_BYTES);
    LE_ERROR_IF(res != LE_OK, "Home PLMN can't be retrives for test case");
    LE_ASSERT(res == LE_OK);
    LE_INFO("Home PLMN is mcc.%s mnc.%s", mccHomeStr, mncHomeStr);

    res = le_mrc_GetRegisterMode(&isManualOrigin,
                    mccStr, LE_MRC_MCC_BYTES, mncStr, LE_MRC_MNC_BYTES);
    LE_ASSERT(res == LE_OK);

    LE_INFO("le_mrc_GetRegisterMode Manual(%c), mcc.%s mnc.%s",
        (isManualOrigin ? 'Y' : 'N'),
        mccStr, mncStr);

    res = le_mrc_SetAutomaticRegisterMode();
    LE_ASSERT(res == LE_OK);

    LE_ASSERT(le_mrc_GetPlatformSpecificRegistrationErrorCode() == 0);

    sleep(5);

    memset(mccStr,0,LE_MRC_MCC_BYTES);
    memset(mncStr,0,LE_MRC_MNC_BYTES);
    res = le_mrc_GetRegisterMode(&isManual, mccStr, LE_MRC_MCC_BYTES, mncStr, LE_MRC_MNC_BYTES);
    LE_ASSERT(res == LE_OK);
    LE_ASSERT(isManual == false);
    LE_INFO("le_mrc_GetRegisterMode Manual(%c), mcc.%s mnc.%s",
        (isManualOrigin ? 'Y' : 'N'),
        mccStr, mncStr);

    res = le_mrc_SetManualRegisterMode(mccHomeStr, mncHomeStr);
    LE_INFO("le_mrc_SetManualRegisterMode %s,%s return %d",mccHomeStr, mncHomeStr, res);
    LE_ASSERT(res == LE_OK);

    sleep(5);

    memset(mccStr,0,LE_MRC_MCC_BYTES);
    memset(mncStr,0,LE_MRC_MNC_BYTES);
    res = le_mrc_GetRegisterMode(&isManual, mccStr, LE_MRC_MCC_BYTES, mncStr, LE_MRC_MNC_BYTES);
    LE_ASSERT(res == LE_OK);
    LE_ASSERT(isManual == true);
    cmpRes = strcmp(mccHomeStr, mccStr);
    LE_WARN_IF(cmpRes, "Doesn't match mccHomeStr (%s) mccStr (%s)",mccHomeStr, mccStr)
    LE_ASSERT(cmpRes == 0);
    cmpRes = strcmp(mncHomeStr, mncStr);
    LE_WARN_IF(cmpRes, "Doesn't match mncHomeStr (%s) mncStr (%s)",mncHomeStr, mncStr)
    LE_ASSERT(cmpRes == 0);
    LE_INFO("le_mrc_GetRegisterMode Manual(Y), mcc.%s mnc.%s", mccStr, mncStr);

    res = le_mrc_SetAutomaticRegisterMode();
    LE_ASSERT(res == LE_OK);

    sleep(5);

    memset(mccStr,0,LE_MRC_MCC_BYTES);
    memset(mncStr,0,LE_MRC_MNC_BYTES);
    res = le_mrc_GetRegisterMode(&isManual, mccStr, LE_MRC_MCC_BYTES, mncStr, LE_MRC_MNC_BYTES);
    LE_ASSERT(res == LE_OK);
    LE_ASSERT(isManual == false);
    LE_INFO("le_mrc_GetRegisterMode Manual(N)");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Manual selection call back function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyManualSelectionHandler
(
    le_result_t result,
    void* contextPtr
)
{
    LE_INFO("le_mrc_SetManualRegisterModeAsync return %d", result);
    if (result == LE_OK )
    {
        le_sem_Post(ThreadSemaphore);
    }
    else
    {
        LE_ERROR("Failed");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread for test Register mode asynchronous .
 *
 * Test API: le_mrc_SetManualRegisterModeAsync() API test
 */
//--------------------------------------------------------------------------------------------------
static void* MyRegisterModeAsyncThread
(
    void* context   ///< Context
)
{
    le_sim_ConnectService();
    le_mrc_ConnectService();

    LE_INFO("le_mrc_SetManualRegisterModeAsync mcc.%s mnc.%s",
        mccHomeStr, mncHomeStr);

    le_mrc_SetManualRegisterModeAsync(mccHomeStr, mncHomeStr, MyManualSelectionHandler, NULL);

    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Register mode asynchronous.
 * This test doesn't work in roaming.!!
 *
 * le_mrc_SetAutomaticRegisterMode() API test
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_RegisterModeAsync()
{
    char mccStr[LE_MRC_MNC_BYTES] = {0};
    char mncStr[LE_MRC_MNC_BYTES] = {0};
    bool isManual;
    int cmpRes;
    le_result_t res;
    le_clk_Time_t time = {0,0};
    time.sec = 120000;

    res = le_mrc_SetAutomaticRegisterMode();
    LE_ASSERT(res == LE_OK);

    sleep(5);

    // Get the home PLMN to compare results.
    res = le_sim_GetHomeNetworkMccMnc(LE_SIM_EXTERNAL_SLOT_1, mccHomeStr, LE_MRC_MCC_BYTES,
        mncHomeStr, LE_MRC_MNC_BYTES);
    LE_ERROR_IF(res != LE_OK, "Home PLMN can't be retrieve for test case");
    LE_ASSERT(res == LE_OK);
    LE_INFO("Home PLMN is mcc.%s mnc.%s", mccHomeStr, mncHomeStr);

    // Init the semaphore for asynchronous callback
    ThreadSemaphore = le_sem_Create("HandlerSem",0);

    RegistrationThreadRef = le_thread_Create("CallBack", MyRegisterModeAsyncThread, NULL);
    le_thread_Start(RegistrationThreadRef);

    // Wait for complete asynchronous registration
    res = le_sem_WaitWithTimeOut(ThreadSemaphore, time);

    LE_ERROR_IF(res != LE_OK, "SYNC FAILED");
    le_thread_Cancel(RegistrationThreadRef);
    le_sem_Delete(ThreadSemaphore);

    memset(mccStr,0,LE_MRC_MCC_BYTES);
    memset(mncStr,0,LE_MRC_MNC_BYTES);
    res = le_mrc_GetRegisterMode(&isManual, mccStr, LE_MRC_MCC_BYTES, mncStr, LE_MRC_MNC_BYTES);
    LE_ASSERT(res == LE_OK);
    LE_ASSERT(isManual == true);
    cmpRes = strcmp(mccHomeStr, mccStr);
    LE_WARN_IF(cmpRes, "Doesn't match mccHomeStr (%s) mccStr (%s)",mccHomeStr, mccStr)
    LE_ASSERT(cmpRes == 0);
    cmpRes = strcmp(mncHomeStr, mncStr);
    LE_WARN_IF(cmpRes, "Doesn't match mncHomeStr (%s) mncStr (%s)",mncHomeStr, mncStr)
    LE_ASSERT(cmpRes == 0);
    LE_INFO("le_mrc_GetRegisterMode %c, mcc.%s mnc.%s",
        (isManual ? 'Y':'N'), mccStr, mncStr);

    sleep(5);
    res = le_mrc_SetAutomaticRegisterMode();
    LE_ASSERT(res == LE_OK);

    sleep(5);
}
//! [Register]

//! [RAT Preferences]
//--------------------------------------------------------------------------------------------------
/**
 * Test: rat preferences mode. Module must supported GSM and LTEs
 *
 * le_mrc_GetRatPreferences() API test
 * le_mrc_SetRatPreferences() API test
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_RatPreferences()
{
    le_result_t res;
    le_mrc_RatBitMask_t bitMask = 0;
    le_mrc_RatBitMask_t bitMaskOrigin = 0;

#define  PRINT_RAT(x)   LE_INFO("Rat preferences %02X=> CDMA.%c GSM.%c LTE.%c UMTS.%c", x,\
                ( (x & LE_MRC_BITMASK_RAT_CDMA) ? 'Y' : 'N'),\
                ( (x & LE_MRC_BITMASK_RAT_GSM) ? 'Y' : 'N'),\
                ( (x & LE_MRC_BITMASK_RAT_LTE) ? 'Y' : 'N'),\
                ( (x & LE_MRC_BITMASK_RAT_UMTS) ? 'Y' : 'N') );

    // Get the current rat preference.
    res = le_mrc_GetRatPreferences(&bitMaskOrigin);
    LE_ASSERT(res == LE_OK);
    if (LE_MRC_BITMASK_RAT_ALL == bitMaskOrigin)
    {
        LE_INFO("Rat preferences %02X=> LE_MRC_BITMASK_RAT_ALL", bitMaskOrigin);
    }
    else
    {
        PRINT_RAT(bitMaskOrigin);
    }

    res = le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_LTE);
    LE_ASSERT(res == LE_OK);
    res = le_mrc_GetRatPreferences(&bitMask);
    if (LE_MRC_BITMASK_RAT_ALL == bitMaskOrigin)
    {
        LE_INFO("Rat preferences %02X=> LE_MRC_BITMASK_RAT_ALL", bitMaskOrigin);
    }
    else
    {
        PRINT_RAT(bitMaskOrigin);
    }
    LE_ASSERT(res == LE_OK);
    LE_ASSERT(bitMask == LE_MRC_BITMASK_RAT_LTE);

    res = le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_GSM);
    LE_ASSERT(res == LE_OK);
    res = le_mrc_GetRatPreferences(&bitMask);
    if (LE_MRC_BITMASK_RAT_ALL == bitMaskOrigin)
    {
        LE_INFO("Rat preferences %02X=> LE_MRC_BITMASK_RAT_ALL", bitMaskOrigin);
    }
    else
    {
        PRINT_RAT(bitMaskOrigin);
    }
    LE_ASSERT(res == LE_OK);
    LE_ASSERT(bitMask == LE_MRC_BITMASK_RAT_GSM);

    if (bitMaskOrigin & LE_MRC_BITMASK_RAT_UMTS)
    {
        res = le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_UMTS);
        LE_ASSERT(res == LE_OK);
        res = le_mrc_GetRatPreferences(&bitMask);
        if (LE_MRC_BITMASK_RAT_ALL == bitMaskOrigin)
        {
            LE_INFO("Rat preferences %02X=> LE_MRC_BITMASK_RAT_ALL", bitMaskOrigin);
        }
        else
        {
            PRINT_RAT(bitMaskOrigin);
        }
        LE_ASSERT(res == LE_OK);
        LE_ASSERT(bitMask == LE_MRC_BITMASK_RAT_UMTS);
    }

    res = le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_ALL);
    LE_ASSERT(res == LE_OK);
    res = le_mrc_GetRatPreferences(&bitMask);
    if (LE_MRC_BITMASK_RAT_ALL == bitMaskOrigin)
    {
        LE_INFO("Rat preferences %02X=> LE_MRC_BITMASK_RAT_ALL", bitMaskOrigin);
    }
    else
    {
        PRINT_RAT(bitMaskOrigin);
    }
    LE_ASSERT(res == LE_OK);

    if (bitMaskOrigin & LE_MRC_BITMASK_RAT_CDMA)
    {
        res = le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_CDMA);
        LE_ASSERT(res == LE_OK);
        res = le_mrc_GetRatPreferences(&bitMask);
        if (LE_MRC_BITMASK_RAT_ALL == bitMaskOrigin)
        {
            LE_INFO("Rat preferences %02X=> LE_MRC_BITMASK_RAT_ALL", bitMaskOrigin);
        }
        else
        {
            PRINT_RAT(bitMaskOrigin);
        }
        LE_ASSERT(res == LE_OK);
        LE_ASSERT(bitMask == LE_MRC_BITMASK_RAT_CDMA);
    }

    res = le_mrc_SetRatPreferences(bitMaskOrigin);
    LE_ASSERT(res == LE_OK);
    res = le_mrc_GetRatPreferences(&bitMask);
    if (LE_MRC_BITMASK_RAT_ALL == bitMaskOrigin)
    {
        LE_INFO("Rat preferences %02X=> LE_MRC_BITMASK_RAT_ALL", bitMaskOrigin);
    }
    else
    {
        PRINT_RAT(bitMaskOrigin);
    }
    LE_ASSERT(res == LE_OK);
    LE_ASSERT(bitMask == bitMaskOrigin);
}
//! [RAT Preferences]


//! [Network Scan]
//--------------------------------------------------------------------------------------------------
/**
 * Read scan information
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReadScanInfo
(
    le_mrc_ScanInformationRef_t     scanInfoRef
)
{
    le_mrc_Rat_t rat;
    bool boolTest;
    le_result_t res;
    char mcc[LE_MRC_MCC_BYTES] = {0};
    char mnc[LE_MRC_MNC_BYTES] = {0};
    char nameStr[100] = {0};

    res = le_mrc_GetCellularNetworkMccMnc(scanInfoRef, mcc, LE_MRC_MCC_BYTES, mnc, LE_MRC_MCC_BYTES);
    LE_ASSERT(res == LE_OK);

    res = le_mrc_GetCellularNetworkName(scanInfoRef, nameStr, 1);
    LE_ASSERT(res == LE_OVERFLOW);
    res = le_mrc_GetCellularNetworkName(scanInfoRef, nameStr, 100);;
    LE_ASSERT(res == LE_OK);
    LE_INFO("1st cellular network name.%s", nameStr);

    rat = le_mrc_GetCellularNetworkRat(scanInfoRef);
    LE_ASSERT((rat>=LE_MRC_RAT_UNKNOWN) && (rat<=LE_MRC_RAT_LTE));
    LE_INFO("le_mrc_GetCellularNetworkRat returns rat %d", rat);

    boolTest = le_mrc_IsCellularNetworkInUse(scanInfoRef);
    LE_INFO("IsCellularNetworkInUse is %s", ( (boolTest) ? "true" : "false"));

    boolTest = le_mrc_IsCellularNetworkAvailable(scanInfoRef);
    LE_INFO("le_mrc_IsCellularNetworkAvailable is %s", ( (boolTest) ? "true" : "false"));

    boolTest = le_mrc_IsCellularNetworkHome(scanInfoRef);
    LE_INFO("le_mrc_IsCellularNetworkHome is %s", ( (boolTest) ? "true" : "false"));

    boolTest = le_mrc_IsCellularNetworkForbidden(scanInfoRef);
    LE_INFO("le_mrc_IsCellularNetworkForbidden is %s", ( (boolTest) ? "true" : "false"));
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Cellular Network Scan.
 *
 * le_mrc_PerformCellularNetworkScan() API test
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_PerformCellularNetworkScan()
{
    le_result_t res;
    le_mrc_RatBitMask_t bitMaskOrigin = 0;
    le_mrc_ScanInformationListRef_t scanInfoListRef = NULL;
    le_mrc_ScanInformationRef_t     scanInfoRef = NULL;

    // Get the current rat preference.
    res = le_mrc_GetRatPreferences(&bitMaskOrigin);
    LE_ASSERT(res == LE_OK);

    if ((bitMaskOrigin & LE_MRC_BITMASK_RAT_GSM) || (bitMaskOrigin == LE_MRC_BITMASK_RAT_ALL))
    {
        LE_INFO("Perform scan on GSM");
        scanInfoListRef = le_mrc_PerformCellularNetworkScan(LE_MRC_BITMASK_RAT_GSM);
    }
    else if (bitMaskOrigin & LE_MRC_BITMASK_RAT_UMTS)
    {
        LE_INFO("Perform scan on UMTS");
        scanInfoListRef = le_mrc_PerformCellularNetworkScan(LE_MRC_BITMASK_RAT_UMTS);
    }
    LE_ASSERT(scanInfoListRef != NULL);

    scanInfoRef = le_mrc_GetFirstCellularNetworkScan(scanInfoListRef);
    LE_ASSERT(scanInfoRef != NULL);
    ReadScanInfo(scanInfoRef);

    while ((scanInfoRef = le_mrc_GetNextCellularNetworkScan(scanInfoListRef)) != NULL)
    {
        ReadScanInfo(scanInfoRef);
    }

    le_mrc_DeleteCellularNetworkScan(scanInfoListRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Cellular Network Scan handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyNetworkScanHandler
(
    le_mrc_ScanInformationListRef_t listRef,
    void* contextPtr
)
{
    le_mrc_ScanInformationRef_t     scanInfoRef = NULL;

    LE_ASSERT(listRef != NULL);

    scanInfoRef = le_mrc_GetFirstCellularNetworkScan(listRef);
    LE_ASSERT(scanInfoRef != NULL);
    ReadScanInfo(scanInfoRef);

    while ((scanInfoRef = le_mrc_GetNextCellularNetworkScan(listRef)) != NULL)
    {
        ReadScanInfo(scanInfoRef);
    }

    le_mrc_DeleteCellularNetworkScan(listRef);

    le_sem_Post(ThreadSemaphore);
}


//--------------------------------------------------------------------------------------------------
/**
 * Thread for asynchronous Network scan test.
 *
 * Test API: le_mrc_PerformCellularNetworkScanAsync() API test
 */
//--------------------------------------------------------------------------------------------------
static void* MyNetworkScanAsyncThread
(
    void* context   ///< See parameter documentation above.
)
{
    le_mrc_RatBitMask_t bitMaskOrigin;

    le_mrc_ConnectService();

    // Get the current rat preference.
    le_result_t res = le_mrc_GetRatPreferences(&bitMaskOrigin);
    LE_ASSERT(res == LE_OK);

    if ((bitMaskOrigin & LE_MRC_BITMASK_RAT_GSM) || (bitMaskOrigin == LE_MRC_BITMASK_RAT_ALL))
    {
        LE_INFO("Perform scan on GSM");
        le_mrc_PerformCellularNetworkScanAsync(LE_MRC_BITMASK_RAT_GSM,
            MyNetworkScanHandler, NULL);
    }
    else if (bitMaskOrigin & LE_MRC_BITMASK_RAT_UMTS)
    {
        LE_INFO("Perform scan on UMTS");
        le_mrc_PerformCellularNetworkScanAsync(LE_MRC_BITMASK_RAT_UMTS,
            MyNetworkScanHandler, NULL);
    }

    le_event_RunLoop();
    return NULL;
}
//! [Network Scan]

//--------------------------------------------------------------------------------------------------
/**
 * Test: Cellular Network Scan asynchronous.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_PerformCellularNetworkScanAsync()
{
    le_result_t res;
    le_clk_Time_t time = {0,0};
    time.sec = 120000;

    // Init the semaphore for asynchronous callback
    ThreadSemaphore = le_sem_Create("HandlerSem",0);

    RegistrationThreadRef = le_thread_Create("CallBack", MyNetworkScanAsyncThread, NULL);
    le_thread_Start(RegistrationThreadRef);

    // Wait for complete asynchronous registration
    res = le_sem_WaitWithTimeOut(ThreadSemaphore, time);
    LE_ERROR_IF(res != LE_OK, "SYNC FAILED");
    le_thread_Cancel(RegistrationThreadRef);

    le_sem_Delete(ThreadSemaphore);

    sleep(5);
}


//! [Band Preferences]
//--------------------------------------------------------------------------------------------------
/**
 * Test: 2G/3G band Preferences mode.
 *
 * le_mrc_GetBandPreferences() API test
 * le_mrc_SetBandPreferences() API test
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_BandPreferences()
{
    le_result_t res;
    le_mrc_BandBitMask_t bandMask = 0;
    le_mrc_BandBitMask_t bandMaskOrigin = 0;

#define  PRINT_BAND(x)   LE_INFO("Band preferences 0x%016"PRIX64, x);

    // Get the current rat preference.
    res = le_mrc_GetBandPreferences(&bandMaskOrigin);
    LE_ASSERT(res == LE_OK);
    PRINT_BAND(bandMaskOrigin);

    LE_WARN_IF(( bandMaskOrigin == 0), "le_mrc_GetBandPreferences bandMaskOrigin = 0");

    if (bandMaskOrigin != 0)
    {
        res = le_mrc_SetBandPreferences(bandMaskOrigin);
        LE_ASSERT(res == LE_OK);

        // Get the current rat preference.
        res = le_mrc_GetBandPreferences(&bandMask);
        PRINT_BAND(bandMask);
        LE_ASSERT(res == LE_OK);
        LE_ASSERT(bandMask == bandMaskOrigin);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Lte band Preferences mode.
 *
 * le_mrc_GetLteBandPreferences() API test
 * le_mrc_SetLteBandPreferences() API test
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_LteBandPreferences()
{
    le_result_t res;
    le_mrc_LteBandBitMask_t bandMask = 0;
    le_mrc_LteBandBitMask_t bandMaskOrigin = 0;

#define  PRINT_BANDLTE(x)   LE_INFO("LTE Band MRC preferences 0x%016X", x);

    // Get the current rat preference.
    res = le_mrc_GetLteBandPreferences(&bandMaskOrigin);
    LE_ASSERT(res == LE_OK);
    PRINT_BANDLTE(bandMaskOrigin);

    LE_WARN_IF(( bandMaskOrigin == 0), "Testle_mrc_LteBandPreferences bandMaskOrigin = 0");

    if (bandMaskOrigin != 0)
    {
        res = le_mrc_SetLteBandPreferences(bandMaskOrigin);
        LE_ASSERT(res == LE_OK);

        // Get the current rat preference.
        res = le_mrc_GetLteBandPreferences(&bandMask);
        PRINT_BANDLTE(bandMask);
        LE_ASSERT(res == LE_OK);
        LE_ASSERT(bandMask == bandMaskOrigin);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: TD-SCDMA band Preferences mode.
 *
 * le_mrc_GetTdScdmaBandPreferences() API test
 * le_mrc_SetTdScdmaBandPreferences() API test
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_TdScdmaBandPreferences()
{
    le_result_t res;
    le_mrc_TdScdmaBandBitMask_t bandMask = 0;
    le_mrc_TdScdmaBandBitMask_t bandMaskOrigin = 0;

#define  PRINT_BANDCDMA(x)   LE_INFO("TD-SCDMA Band preferences 0x%016X", x);

    // Get the current rat preference.
    res = le_mrc_GetTdScdmaBandPreferences(&bandMaskOrigin);
    LE_ASSERT(res == LE_OK);
    PRINT_BANDCDMA(bandMaskOrigin);

    LE_WARN_IF(( bandMaskOrigin == 0), "le_mrc_GetTdScdmaBandPreferences bandMaskOrigin = 0");

    if (bandMaskOrigin != 0)
    {
        res = le_mrc_SetTdScdmaBandPreferences(bandMaskOrigin);
        LE_ASSERT(res == LE_OK);

        // Get the current rat preference.
        res = le_mrc_GetTdScdmaBandPreferences(&bandMask);
        LE_ASSERT(res == LE_OK);
        PRINT_BANDCDMA(bandMask);
        LE_ASSERT(bandMask == bandMaskOrigin);
    }
}
//! [Band Preferences]


//! [Signal Quality]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Network Registration State + Signal Quality.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_GetStateAndQual()
{
    le_result_t           res;
    le_mrc_NetRegState_t  state;
    uint32_t              quality;

    res = le_mrc_GetNetRegState(&state);
    LE_ASSERT(res == LE_OK);
    if (res == LE_OK)
    {
        LE_ASSERT((state>=LE_MRC_REG_NONE) && (state<=LE_MRC_REG_UNKNOWN));
    }

    res = le_mrc_GetSignalQual(&quality);
    LE_ASSERT(res == LE_OK);
    if (res == LE_OK)
    {
        LE_ASSERT((quality>=0) && (quality<=5));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Get Signal Metrics.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_GetSignalMetrics()
{
    le_result_t   res;
    le_mrc_Rat_t  rat;
    int32_t rxLevel = 0;
    uint32_t er = 0;
    int32_t ecio = 0;
    int32_t rscp = 0;
    int32_t sinr = 0;
    int32_t rsrq = 0;
    int32_t rsrp = 0;
    int32_t snr = 0;
    int32_t io = 0;

    le_mrc_MetricsRef_t metricsRef = le_mrc_MeasureSignalMetrics();
    LE_ASSERT(metricsRef != NULL);

    rat = le_mrc_GetRatOfSignalMetrics(metricsRef);
    LE_INFO("RAT of signal metrics is %d",rat);
    switch(rat)
    {
        case LE_MRC_RAT_GSM:
            res = le_mrc_GetGsmSignalMetrics(metricsRef, &rxLevel, &er);
            LE_ASSERT(res == LE_OK);
            LE_INFO("GSM metrics rxLevel.%ddBm, er.%d", rxLevel, er);
            break;

        case LE_MRC_RAT_UMTS:
            res = le_mrc_GetUmtsSignalMetrics(metricsRef, &rxLevel, &er, &ecio, &rscp, &sinr);
            LE_ASSERT(res == LE_OK);
            LE_INFO("UMTS metrics rxLevel.%ddBm, er.%d, ecio.%010.1fdB, rscp.%ddBm, sinr.%ddB",
                    rxLevel, er, ((double)ecio/10), rscp, sinr);
            break;

        case LE_MRC_RAT_LTE:
            res = le_mrc_GetLteSignalMetrics(metricsRef, &rxLevel, &er, &rsrq, &rsrp, &snr);
            LE_ASSERT(res == LE_OK);
            LE_INFO("LTE metrics rxLevel.%ddBm, er.%d, rsrq.%010.1fdB, "
                    "rsrp.%010.1fdBm, snr.%010.1fdB",
                    rxLevel, er, ((double)rsrq/10), ((double)rsrp/10), ((double)snr/10));
            break;

        case LE_MRC_RAT_CDMA:
            res = le_mrc_GetCdmaSignalMetrics(metricsRef,  &rxLevel, &er, &ecio, &sinr, &io);
            LE_ASSERT(res == LE_OK);
            LE_INFO("CDMA metrics rxLevel.%ddBm, er.%d, ecio.%010.1fdB, "
                    "sinr.%ddB, io.%ddBm",
                    rxLevel, er, ((double)ecio/10), sinr, io);
            break;

        default:
            LE_FATAL("Unknown RAT!");
            break;
    }

    le_mrc_DeleteSignalMetrics(metricsRef);
}
//! [Signal Quality]


//! [Neighbor Cells]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Neighbor Cells Information.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_GetNeighboringCellsInfo()
{
    le_result_t res;
    le_mrc_NeighborCellsRef_t ngbrRef;
    le_mrc_CellInfoRef_t cellRef;
    uint32_t i = 0;
    uint32_t cid = 0;
    uint32_t lac = 0;
    int32_t rxLevel = 0;
    le_mrc_Rat_t rat = 0;
    int32_t ecio = 0;
    int32_t intraRsrp = 0;
    int32_t intraRsrq = 0;
    int32_t interRsrp = 0;
    int32_t interRsrq = 0;

    LE_INFO("Start Testle_mrc_GetNeighborCellsInfo");

    ngbrRef = le_mrc_GetNeighborCellsInfo();
    LE_ASSERT(ngbrRef);

    if (ngbrRef)
    {
        i = 0;

        cellRef = le_mrc_GetFirstNeighborCellInfo(ngbrRef);
        LE_ASSERT(cellRef);
        cid = le_mrc_GetNeighborCellId(cellRef);
        lac = le_mrc_GetNeighborCellLocAreaCode(cellRef);
        rxLevel = le_mrc_GetNeighborCellRxLevel(cellRef);
        rat = le_mrc_GetNeighborCellRat(cellRef);
        LE_INFO("Cell #%d, cid.%d, lac.%d, rxLevel.%ddBm, RAT.%d", i, cid, lac, rxLevel, rat);
        // Specific values for UMTS and LTE
        switch(rat)
        {
            case LE_MRC_RAT_UMTS:
                ecio = le_mrc_GetNeighborCellUmtsEcIo(cellRef);
                LE_INFO("Cell #%d, UMTS EcIo.%010.1fdB", i, ((double)ecio/10));
                break;

            case LE_MRC_RAT_LTE:
                res = le_mrc_GetNeighborCellLteIntraFreq(cellRef, &intraRsrq, &intraRsrp);
                LE_ASSERT(res == LE_OK);
                res = le_mrc_GetNeighborCellLteInterFreq(cellRef, &interRsrq, &interRsrp);
                LE_ASSERT(res == LE_OK);

                LE_INFO("Cell #%d, LTE Intra-RSRQ.%010.1fdB, Intra-RSRP.%010.1fdBm, "
                        "Inter-RSRQ.%010.1fdB, Inter-RSRP.%010.1fdBm",
                        i, ((double)intraRsrq/10), ((double)intraRsrp/10),
                        ((double)interRsrq/10), ((double)interRsrp/10));
                break;

            default:
                LE_INFO("Nothing more to display");
                break;
        }

        while ((cellRef = le_mrc_GetNextNeighborCellInfo(ngbrRef)) != NULL)
        {
            i++;
            cid = le_mrc_GetNeighborCellId(cellRef);
            lac = le_mrc_GetNeighborCellLocAreaCode(cellRef);
            rxLevel = le_mrc_GetNeighborCellRxLevel(cellRef);
            rat = le_mrc_GetNeighborCellRat(cellRef);
            LE_INFO("Cell #%d, cid.%d, lac.%d, rxLevel.%ddBm, RAT.%d", i, cid, lac, rxLevel, rat);
            // Specific values for UMTS and LTE
            switch(rat)
            {
                case LE_MRC_RAT_UMTS:
                    ecio = le_mrc_GetNeighborCellUmtsEcIo(cellRef);
                    LE_INFO("Cell #%d, UMTS EcIo.%010.1fdB", i, ((double)ecio/10));
                    break;

                case LE_MRC_RAT_LTE:
                    res = le_mrc_GetNeighborCellLteIntraFreq(cellRef, &intraRsrq, &intraRsrp);
                    LE_ASSERT(res == LE_OK);
                    res = le_mrc_GetNeighborCellLteInterFreq(cellRef, &interRsrq, &interRsrp);
                    LE_ASSERT(res == LE_OK);

                    LE_INFO("Cell #%d, LTE Intra-RSRQ.%010.1fdB, Intra-RSRP.%010.1fdBm, "
                            "Inter-RSRQ.%010.1fdB, Inter-RSRP.%010.1fdBm",
                            i, ((double)intraRsrq/10), ((double)intraRsrp/10),
                            ((double)interRsrq/10), ((double)interRsrp/10));
                    break;

                default:
                    LE_INFO("Nothing more to display");
                    break;
            }
        }

        le_mrc_DeleteNeighborCellsInfo(ngbrRef);
    }
}
//! [Neighbor Cells]


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for GSM Signal Strength change Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestGsmSsHandler
(
    int32_t     ss,
    void*       contextPtr
)
{
    LE_INFO("New GSM Signal Strength change: %ddBm", ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for UMTS Signal Strength change Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestUmtsSsHandler
(
    int32_t     ss,
    void*       contextPtr
)
{
    LE_INFO("New UMTS Signal Strength change: %ddBm", ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for LTE Signal Strength change Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestLteSsHandler
(
    int32_t     ss,
    void*       contextPtr
)
{
    LE_INFO("New LTE Signal Strength change: %ddBm", ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for CDMA Signal Strength change Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestCdmaSsHandler
(
    int32_t     ss,
    void*       contextPtr
)
{
    LE_INFO("New CDMA Signal Strength change: %ddBm", ss);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Signal Strength change handling.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_SsHdlr()
{
    le_mrc_SignalStrengthChangeHandlerRef_t testHdlrRef=NULL;

    testHdlrRef = le_mrc_AddSignalStrengthChangeHandler(LE_MRC_RAT_GSM,
                                                        -80,
                                                        -70,
                                                        TestGsmSsHandler,
                                                        NULL);
    LE_ASSERT(testHdlrRef);

    testHdlrRef = le_mrc_AddSignalStrengthChangeHandler(LE_MRC_RAT_UMTS,
                                                        -80,
                                                        -70,
                                                        TestUmtsSsHandler,
                                                        NULL);
    LE_ASSERT(testHdlrRef);

    testHdlrRef = le_mrc_AddSignalStrengthChangeHandler(LE_MRC_RAT_LTE,
                                                        -80,
                                                        -70,
                                                        TestLteSsHandler,
                                                        NULL);
    LE_ASSERT(testHdlrRef);

    testHdlrRef = le_mrc_AddSignalStrengthChangeHandler(LE_MRC_RAT_CDMA,
                                                        -80,
                                                        -70,
                                                        TestCdmaSsHandler,
                                                        NULL);
    LE_ASSERT(testHdlrRef);
}


//! [Loc information]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Location information.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_GetLocInfo()
{
    uint32_t  cellId, lac;
    uint16_t  tac;

    cellId = le_mrc_GetServingCellId();
    LE_INFO("le_mrc_GetServingCellId returns cellId.%d", cellId);
    lac = le_mrc_GetServingCellLocAreaCode();
    LE_INFO("le_mrc_GetServingCellLocAreaCode returns lac.%d",lac);
    tac = le_mrc_GetServingCellLteTracAreaCode();
    LE_INFO("le_mrc_GetServingCellLteTracAreaCode returns Tac.0x%X (%d)", tac, tac);
}
//! [Loc information]


//--------------------------------------------------------------------------------------------------
/**
 * Test: Current preferred network operators.
 * SIM used must support preferred PLMNs storage (File EF 6f20)
 *
 * le_mrc_GetPreferredOperatorsList() API test
 * le_mrc_GetFirstPreferredOperator() API test
 * le_mrc_GetPreferredOperatorDetails() API test
 * le_mrc_GetNextPreferredOperator() API test
 * le_mrc_DeletePreferredOperatorsList() API test
 * le_mrc_AddPreferredOperator() API test
 * le_mrc_RemovePreferredOperator() API test
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_PreferredPLMN()
{
    le_result_t res;
    char mccStr[LE_MRC_MCC_BYTES] = {0};
    char mncStr[LE_MRC_MNC_BYTES] = {0};
    char saveMccStr[3][5];
    char saveMncStr[3][5];
    le_mrc_RatBitMask_t saveRat[3];

    le_mrc_PreferredOperatorRef_t optRef = NULL;
    le_mrc_RatBitMask_t ratMask;
    int beforeIndex = 0;
    int afterIndex = 0;

    le_mrc_PreferredOperatorListRef_t prefPlmnList = le_mrc_GetPreferredOperatorsList();

    LE_INFO("======== PreferredPLMN Test ========");

    LE_INFO("le_mrc_GetPreferredOperatorsList() started %p", prefPlmnList);
    if (prefPlmnList)
    {
        optRef = le_mrc_GetFirstPreferredOperator(prefPlmnList);
        while (optRef)
        {
            res = le_mrc_GetPreferredOperatorDetails(optRef,
                mccStr, LE_MRC_MCC_BYTES,
                mncStr, LE_MRC_MNC_BYTES,
                &ratMask);
            LE_ASSERT(res == LE_OK);

            res = le_mrc_GetPreferredOperatorDetails(optRef,
                mccStr, LE_MRC_MCC_BYTES-1,
                mncStr, LE_MRC_MNC_BYTES,
                &ratMask);
            LE_ASSERT(res == LE_OVERFLOW);


            res = le_mrc_GetPreferredOperatorDetails(optRef,
                mccStr, LE_MRC_MCC_BYTES,
                mncStr, LE_MRC_MNC_BYTES-1,
                &ratMask);
            LE_ASSERT(res == LE_OVERFLOW);

            if(beforeIndex < 3)
            {
                strcpy(saveMccStr[beforeIndex], mccStr);
                strcpy(saveMncStr[beforeIndex], mncStr);
                saveRat[beforeIndex] = ratMask;
                LE_INFO("Save (%d) mcc=%s mnc=%s rat=%d", beforeIndex,
                    saveMccStr[beforeIndex],
                    saveMncStr[beforeIndex],
                    saveRat[beforeIndex] );
            }
            else
            {
                LE_INFO("Get_detail Loop(%d) mcc.%s mnc %s, rat.%08X, GSM %c, LTE %c, UMTS %c",
                    beforeIndex, mccStr, mncStr, ratMask,
                    (ratMask & LE_MRC_BITMASK_RAT_GSM ? 'Y':'N'),
                    (ratMask & LE_MRC_BITMASK_RAT_LTE ? 'Y':'N'),
                    (ratMask & LE_MRC_BITMASK_RAT_UMTS ? 'Y':'N')
                );
            }

            optRef = le_mrc_GetNextPreferredOperator(prefPlmnList);

            beforeIndex++;
        }
        LE_INFO("No more preferred PLMN operator present in the modem List %d Displayed",
            beforeIndex);
        le_mrc_DeletePreferredOperatorsList(prefPlmnList);
    }
    else
    {
        LE_WARN("=== PreferredPLMN Test No Preferred PLMN list present in the SIM ====");
        LE_INFO("======== PreferredPLMN Test  N/A ========");
        return;
    }
    LE_INFO("le_mrc_GetPreferredOperatorsList() end");

    if (beforeIndex >= 3)
    {
        LE_INFO("Remove 3 entries in the network opreator list");
        LE_INFO("Remove third entries for the test and restore them after");
        res = le_mrc_RemovePreferredOperator(saveMccStr[0], saveMncStr[0]);
        LE_ASSERT(res == LE_OK);
        res = le_mrc_RemovePreferredOperator(saveMccStr[1], saveMncStr[1]);
        LE_ASSERT(res == LE_OK);
        res = le_mrc_RemovePreferredOperator(saveMccStr[2], saveMncStr[2]);
        LE_ASSERT(res == LE_OK);
    }
    else
    {
        LE_WARN("=== Less than 3 entries present in the SIM ====");
    }

    LE_INFO("le_mrc_AddPreferredOperator() started");
    res = le_mrc_AddPreferredOperator("208", "10", LE_MRC_BITMASK_RAT_ALL);
    LE_ASSERT(res == LE_OK);
    res = le_mrc_AddPreferredOperator("208", "10", LE_MRC_BITMASK_RAT_UMTS);
    LE_ASSERT(res == LE_OK);

    res = le_mrc_AddPreferredOperator("311", "070", LE_MRC_BITMASK_RAT_ALL);
    LE_ASSERT(res == LE_OK);
    res = le_mrc_AddPreferredOperator("311", "70", LE_MRC_BITMASK_RAT_ALL);
    LE_ASSERT(res == LE_OK);
    LE_INFO("le_mrc_AddPreferredOperator() end");

    LE_INFO("le_mrc_RemovePreferredOperator() started");
    res = le_mrc_RemovePreferredOperator("208", "10");
    LE_ASSERT(res == LE_OK);
    res = le_mrc_RemovePreferredOperator("311", "070");
    LE_ASSERT(res == LE_OK);
    res = le_mrc_RemovePreferredOperator("311", "70");
    LE_ASSERT(res == LE_OK);

    res = le_mrc_RemovePreferredOperator("311", "70");
    LE_ASSERT(res == LE_FAULT);
    LE_INFO("le_mrc_RemovePreferredOperator() end");

    prefPlmnList = le_mrc_GetPreferredOperatorsList();
    LE_ASSERT(prefPlmnList != NULL);

    optRef = le_mrc_GetFirstPreferredOperator(prefPlmnList);
    while (optRef)
    {
        res = le_mrc_GetPreferredOperatorDetails(optRef,
            mccStr, LE_MRC_MCC_BYTES,
            mncStr, LE_MRC_MNC_BYTES,
            &ratMask);

        LE_ASSERT(res == LE_OK);
        LE_INFO("Get_detail Loop(%d) mcc.%s mnc %s, rat.%08X,  GSM %c, LTE %c, UMTS %c",
            ++afterIndex, mccStr, mncStr, ratMask,
            (ratMask & LE_MRC_BITMASK_RAT_GSM ? 'Y':'N'),
            (ratMask & LE_MRC_BITMASK_RAT_LTE ? 'Y':'N'),
            (ratMask & LE_MRC_BITMASK_RAT_UMTS ? 'Y':'N')
        );

        optRef = le_mrc_GetNextPreferredOperator(prefPlmnList);
    }

    if (beforeIndex >= 3)
    {
        LE_INFO("Restore third entries for the test and restore them after");
        res = le_mrc_AddPreferredOperator(saveMccStr[0], saveMncStr[0], saveRat[0]);
        LE_ASSERT(res == LE_OK);
        res = le_mrc_AddPreferredOperator(saveMccStr[1], saveMncStr[1], saveRat[1]);
        LE_ASSERT(res == LE_OK);
        res = le_mrc_AddPreferredOperator(saveMccStr[1], saveMncStr[1], saveRat[2]);
        LE_ASSERT(res == LE_OK);
    }

    LE_INFO("No more preferred PLMN operator present in the modem List after %d, before %d",
        afterIndex, beforeIndex);
    le_mrc_DeletePreferredOperatorsList(prefPlmnList);

    LE_INFO("======== PreferredPLMN Test PASSED ========");
}


//! [Band Capabilities]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Get platform band capabilities.
 *
 * le_mrc_GetBandCapabilities() API test
 * le_mrc_GetLteBandCapabilities() API test
 * le_mrc_GetTdScdmaBandCapabilities() API test
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_GetBandCapabilities()
{
    le_mrc_BandBitMask_t            bands = 0;
    le_mrc_LteBandBitMask_t         lteBands = 0;
    le_mrc_TdScdmaBandBitMask_t     tdScdmaBands = 0;

    // Not supported on all platform
    if (le_mrc_GetBandCapabilities(&bands) == LE_OK)
    {
        LE_INFO("Get 2G/3G Band Capabilities bit mask: 0x%016"PRIX64, (uint64_t)bands);
    }
    else
    {
        LE_WARN("le_mrc_GetBandCapabilities failed");
    }

    if (le_mrc_GetLteBandCapabilities(&lteBands) == LE_OK)
    {
        LE_INFO("Get LTE Band Capabilities bit mask: 0x%016"PRIX64, (uint64_t)lteBands);
    }
    else
    {
        LE_WARN("le_mrc_GetLteBandCapabilities failed");
    }

    if (le_mrc_GetTdScdmaBandCapabilities(&tdScdmaBands) == LE_OK)
    {
        LE_INFO("Get TD-SCDMA Band Capabilities bit mask: 0x%016"PRIX64, (uint64_t)tdScdmaBands);
    }
    else
    {
        LE_WARN("le_mrc_GetTdScdmaBandCapabilities failed");
    }
}
//! [Band Capabilities]


//--------------------------------------------------------------------------------------------------
/**
 * Main
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("======== Start MRC Modem Services implementation Test========");

    LE_INFO("======== CSHdlr Test ========");
    Testle_mrc_CSHdlr();
    LE_INFO("======== CSHdlr Test PASSED ========");

    LE_INFO("======== PSHdlr Test ========");
    Testle_mrc_PSHdlr();
    LE_INFO("======== PSHdlr Test PASSED ========");

    LE_INFO("======== PreferredPLMN Test ========");
    Testle_mrc_PreferredPLMN();
    LE_INFO("======== PreferredPLMN Test PASSED ========");

    LE_INFO("======== BandCapabilities Test ========");
    Testle_mrc_GetBandCapabilities();
    LE_INFO("======== BandCapabilities Test PASSED ========");

#if TEST_MRC_POWER
    LE_INFO("======== Power Test ========");
    Testle_mrc_Power();
    LE_INFO("======== Power Test PASSED ========");
#endif

    LE_INFO("======== GetStateAndQual Test ========");
    Testle_mrc_GetStateAndQual();
    LE_INFO("======== GetStateAndQual Test PASSED ========");

    LE_INFO("======== GetRat Test ========");
    Testle_mrc_GetRat();
    LE_INFO("======== GetRat Test PASSED ========");

    LE_INFO("======== GetCSstate Test ========");
    Testle_mrc_GetCSState();
    LE_INFO("======== GetCSstate Test PASSED ========");

    LE_INFO("======== GetPSState Test ========");
    Testle_mrc_GetPSState();
    LE_INFO("======== GetPSState Test PASSED ========");

    LE_INFO("======== Location information Test ========");
    Testle_mrc_GetLocInfo();
    LE_INFO("======== Location information Test PASSED ========");

    LE_INFO("======== GetSignalMetrics Test ========");
    Testle_mrc_GetSignalMetrics();
    LE_INFO("======== GetSignalMetrics Test PASSED ========");

    LE_INFO("======== GetNeighboringCellsInfo Test ========");
    Testle_mrc_GetNeighboringCellsInfo();
    LE_INFO("======== GetNeighboringCellsInfo Test PASSED ========");

    LE_INFO("======== NetRegHdlr Test ========");
    Testle_mrc_NetRegHdlr();
    LE_INFO("======== NetRegHdlr Test PASSED ========");

    LE_INFO("======== RatHdlr Test ========");
    Testle_mrc_RatHdlr();
    LE_INFO("======== RatHdlr Test PASSED ========");

    LE_INFO("======== GetCurrentNetworkMccMnc Test ========");
    Testle_mrc_GetCurrentNetworkMccMnc();
    LE_INFO("======== GetCurrentNetworkMccMnc Test PASSED ========");

    LE_INFO("======== GetCurrentNetworkName Test ========");
    Testle_mrc_GetCurrentNetworkName();
    LE_INFO("======== GetCurrentNetworkName Test PASSED ========");

    LE_INFO("======== RegisterMode Test ========");
    Testle_mrc_RegisterMode();
    LE_INFO("======== RegisterMode Test PASSED ========");

    LE_INFO("======== RegisterModeAsync Test ========");
    Testle_mrc_RegisterModeAsync();
    LE_INFO("======== RegisterModeAsync Test PASSED ========");

    LE_INFO("======== RatPreferences Test ========");
    Testle_mrc_RatPreferences();
    LE_INFO("======== RatPreferences Test PASSED ========");

    LE_INFO("======== PerformCellularNetworkScan Test ========");
    Testle_mrc_PerformCellularNetworkScan();
    LE_INFO("======== PerformCellularNetworkScan Test PASSED ========");

    LE_INFO("======== PerformCellularNetworkScanAsync Test ========");
    Testle_mrc_PerformCellularNetworkScanAsync();
    LE_INFO("======== PerformCellularNetworkScanAsync Test PASSED ========");

    LE_INFO("======== BandPreferences Test ========");
    Testle_mrc_BandPreferences();
    LE_INFO("======== BandPreferences Test PASSED ========");

    LE_INFO("======== BandLtePreferences Test ========");
    Testle_mrc_LteBandPreferences();
    LE_INFO("======== BandLtePreferences Test PASSED ========");

    LE_INFO("======== BandTdScdmaPreferences Test ========");
    Testle_mrc_TdScdmaBandPreferences();
    LE_INFO("======== BandTdScdmaPreferences Test PASSED ========");

    LE_INFO("======== Signal Strength Handler Test ========");
    Testle_mrc_SsHdlr();
    LE_INFO("======== Signal Strength Handler Test PASSED ========");

    LE_INFO("======== Test MRC Modem Services implementation Test SUCCESS ========");

    exit(EXIT_SUCCESS);
}


