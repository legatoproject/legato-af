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
 * Note: for Jamming detection test, make sure the feature is enabled by AT!CUSTOM="JAMENABLE",1
 */

#include "legato.h"
#include <interfaces.h>

//--------------------------------------------------------------------------------------------------
/**
 * Maximum and default values for SAR backoff state
 */
//--------------------------------------------------------------------------------------------------
#define SAR_BACKOFF_STATE_MAX       8
#define SAR_BACKOFF_STATE_DEFAULT   0

//--------------------------------------------------------------------------------------------------
/**
 * Value for sleep
 */
//--------------------------------------------------------------------------------------------------
#define SLEEP_5S  5

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
static le_sem_Ref_t    ThreadSemaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Registration Thread reference.
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t  RegistrationThreadRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Signal Strength Thread reference.
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t  SignalStrengthChangeThreadRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Jamming detection Thread reference.
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t  JammingDetectionTreadRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Home PLMN references
 */
//--------------------------------------------------------------------------------------------------
static char mccHomeStr[LE_MRC_MCC_BYTES] = {0};
static char mncHomeStr[LE_MRC_MCC_BYTES] = {0};

//--------------------------------------------------------------------------------------------------
/**
 * Signal Strength handler reference.
 */
//--------------------------------------------------------------------------------------------------
static le_mrc_SignalStrengthChangeHandlerRef_t SignalHdlrRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * RAT change handler reference.
 */
//--------------------------------------------------------------------------------------------------
static le_mrc_RatChangeHandlerRef_t RatChangeHdlrRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * RAT name map
 */
//--------------------------------------------------------------------------------------------------
static const char* const RatTypeName[] =
{
    [LE_MRC_RAT_UNKNOWN] = "Unknown",
    [LE_MRC_RAT_GSM] = "GSM",
    [LE_MRC_RAT_UMTS] = "UMTS",
    [LE_MRC_RAT_TDSCDMA] = "TD-SCDMA",
    [LE_MRC_RAT_LTE] = "LTE",
    [LE_MRC_RAT_CDMA] = "CDMA",
};

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
    bool changeRat = true;
    LE_INFO("New RAT: %d", rat);

    switch(rat)
    {
        case LE_MRC_RAT_CDMA:
            LE_INFO("Check RatHandler passed, RAT is LE_MRC_RAT_CDMA.");
            break;

        case LE_MRC_RAT_GSM:
            LE_INFO("Check RatHandler passed, RAT is LE_MRC_RAT_GSM.");
            break;

        case LE_MRC_RAT_UMTS:
            LE_INFO("Check RatHandler passed, RAT is LE_MRC_RAT_UMTS.");
            break;

        case LE_MRC_RAT_TDSCDMA:
            LE_INFO("Check RatHandler passed, RAT is LE_MRC_RAT_TDSCDMA.");
            break;

        case LE_MRC_RAT_LTE:
            LE_INFO("Check RatHandler passed, RAT is LE_MRC_RAT_LTE.");
            break;

        default:
            LE_ERROR("Check RatHandler failed, bad RAT.");
            changeRat = false;
            break;
    }

    if ((true == changeRat) && (NULL != ThreadSemaphore))
    {
        le_sem_Post(ThreadSemaphore);
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
    le_mrc_NetRegState_t psState,
    void*        contextPtr
)
{
    LE_INFO("New PS state: %d", psState);
    switch(psState)
    {
        case LE_MRC_REG_HOME:
            LE_INFO("New PS state LE_MRC_REG_HOME");
            break;
        case LE_MRC_REG_ROAMING:
            LE_INFO("New PS state LE_MRC_REG_ROAMING");
            break;
        case LE_MRC_REG_NONE:
            LE_INFO("New PS state LE_MRC_REG_NONE");
            break;
        default:
            LE_ERROR("New PS state unknown PS state %d", psState);
            break;
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
static void Testle_mrc_Power
(
    void
)
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
static void Testle_mrc_GetRat
(
    void
)
{
    le_result_t   res;
    le_mrc_Rat_t  rat;

    res = le_mrc_GetRadioAccessTechInUse(&rat);
    LE_ASSERT(res == LE_OK);

    LE_ASSERT((rat>=LE_MRC_RAT_UNKNOWN) && (rat<=LE_MRC_RAT_LTE));

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
    le_mrc_NetRegState_t  psState;

    LE_ASSERT_OK(le_mrc_GetPacketSwitchedState(&psState));
    switch(psState)
    {
        case LE_MRC_REG_HOME:
            LE_INFO("le_mrc_GetPacketSwitchedState returns LE_MRC_REG_HOME");
            break;
        case LE_MRC_REG_ROAMING:
            LE_INFO("le_mrc_GetPacketSwitchedState returns LE_MRC_REG_ROAMING");
            break;
        case LE_MRC_REG_NONE:
            LE_INFO("le_mrc_GetPacketSwitchedState returns LE_MRC_REG_NONE");
            break;
        default:
            LE_ERROR("le_mrc_GetPacketSwitchedState returns an unknown PS state %d", psState);
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
static void Testle_mrc_NetRegHdlr
(
    void
)
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
static void Testle_mrc_RatHdlr
(
    void
)
{
    le_mrc_RatChangeHandlerRef_t testHdlrRef;

    testHdlrRef = le_mrc_AddRatChangeHandler(TestRatHandler, NULL);
    LE_ASSERT(testHdlrRef);
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
static void Testle_mrc_GetCurrentNetworkMccMnc
(
    void
)
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
static void Testle_mrc_GetCurrentNetworkName
(
    void
)
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
static void Testle_mrc_RegisterMode
(
    void
)
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
static void Testle_mrc_RegisterModeAsync
(
    void
)
{
    char mccStr[LE_MRC_MNC_BYTES] = {0};
    char mncStr[LE_MRC_MNC_BYTES] = {0};
    bool isManual;
    int cmpRes;
    le_result_t res;
    le_clk_Time_t time;
    time.sec = 180;
    time.usec = 0;

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

    sleep(SLEEP_5S);

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

    sleep(SLEEP_5S);
    res = le_mrc_SetAutomaticRegisterMode();
    LE_ASSERT(res == LE_OK);

    sleep(SLEEP_5S);
}
//! [Register]

//! [RAT Preferences]
//--------------------------------------------------------------------------------------------------
/**
 * Display bitmask RAT
 */
//--------------------------------------------------------------------------------------------------
static void PrintRat
(
    le_mrc_RatBitMask_t bitMask
)
{
    if(LE_MRC_BITMASK_RAT_ALL != bitMask)
    {
        LE_INFO("Rat preferences %02X=> CDMA.%c GSM.%c LTE.%c UMTS.%c TD-SCDMA.%c", bitMask,
            ((bitMask & LE_MRC_BITMASK_RAT_CDMA) ? 'Y' : 'N'),
            ((bitMask & LE_MRC_BITMASK_RAT_GSM) ? 'Y' : 'N'),
            ((bitMask & LE_MRC_BITMASK_RAT_LTE) ? 'Y' : 'N'),
            ((bitMask & LE_MRC_BITMASK_RAT_UMTS) ? 'Y' : 'N'),
            ((bitMask & LE_MRC_BITMASK_RAT_TDSCDMA) ? 'Y' : 'N'));
    }
    else
    {
        LE_INFO("Rat preferences => LE_MRC_BITMASK_RAT_ALL");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread for Rat Preferences test
 */
//--------------------------------------------------------------------------------------------------
static void* MyRatPreferencesThread
(
    void* contextPtr   ///< Context
)
{
    le_mrc_ConnectService();

    RatChangeHdlrRef = le_mrc_AddRatChangeHandler(TestRatHandler, NULL);
    LE_ASSERT(RatChangeHdlrRef);

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: rat preferences mode. Module must supported GSM and LTEs
 *
 * le_mrc_GetRatPreferences() API test
 * le_mrc_SetRatPreferences() API test
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_RatPreferences
(
    void
)
{
    le_mrc_RatBitMask_t bitMask = 0;
    le_mrc_RatBitMask_t bitMaskOrigin = 0;
    le_result_t res;
    le_mrc_Rat_t rat;
    bool lteSupported = false;
    le_clk_Time_t time;
    time.sec = 30;
    time.usec = 0;

    // Create a semaphore for asynchronous RAT change indication
    ThreadSemaphore = le_sem_Create("HandlerRatChange", 0);
    // Start a thread to receive RAT change indication.
    RegistrationThreadRef = le_thread_Create("MyRatPreferencesThread",
                                             MyRatPreferencesThread,
                                             NULL);
    le_thread_Start(RegistrationThreadRef);

    // Backup current rat preference.
    LE_ASSERT_OK(le_mrc_GetRatPreferences(&bitMaskOrigin));
    PrintRat(bitMaskOrigin);

    // If current RAT in use is not  LTE, then set RAT to LTE
    // and test if RAT change indication is received.
    res = le_mrc_GetRadioAccessTechInUse(&rat);
    LE_ASSERT(res == LE_OK);

    if (rat != LE_MRC_RAT_LTE)
    {
        LE_INFO("Set RAT from %s to %s", RatTypeName[rat], RatTypeName[LE_MRC_RAT_LTE]);
        res = le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_LTE);
        LE_ASSERT((LE_OK == res) || (LE_UNSUPPORTED == res));

        if (LE_OK == res)
        {
            //LTE supported, waiting for RAT change indication
            lteSupported = true;
            res = le_sem_WaitWithTimeOut(ThreadSemaphore, time);
            LE_ASSERT(res == LE_OK);
            LE_ASSERT_OK(le_mrc_GetRatPreferences(&bitMask));
            PrintRat(bitMask);

            if ((LE_MRC_BITMASK_RAT_LTE != bitMask))
            {
                if ((LE_MRC_BITMASK_RAT_LTE | LE_MRC_BITMASK_RAT_GSM) == bitMask)
                {
                    LE_WARN("LTE only not supported");
                }
            }

            LE_ASSERT(bitMask & LE_MRC_BITMASK_RAT_LTE);
        }
    }
    else
    {
        lteSupported = true;
    }

    // If current RAT in use is not  GSM, then set RAT to GSM
    // and test if RAT change indication is received.
    res = le_mrc_GetRadioAccessTechInUse(&rat);
    LE_ASSERT(res == LE_OK);

    if (rat != LE_MRC_RAT_GSM)
    {
        LE_INFO("Set RAT from %s to %s", RatTypeName[rat], RatTypeName[LE_MRC_RAT_GSM]);
        LE_ASSERT_OK(le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_GSM));

        //GSM supported, waiting for RAT change indication
        res = le_sem_WaitWithTimeOut(ThreadSemaphore, time);
        LE_ASSERT(res == LE_OK);

        LE_ASSERT_OK(le_mrc_GetRatPreferences(&bitMask));
        PrintRat(bitMask);

        LE_ASSERT(LE_MRC_BITMASK_RAT_GSM == bitMask);
    }

    // If current RAT in use is not  UMTS, then set RAT to UMTS
    // and test if RAT change indication is received.
    res = le_mrc_GetRadioAccessTechInUse(&rat);
    LE_ASSERT(res == LE_OK);

    if (rat != LE_MRC_RAT_UMTS)
    {
        LE_INFO("Set RAT from %s to %s", RatTypeName[rat], RatTypeName[LE_MRC_RAT_UMTS]);
        LE_ASSERT_OK(le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_UMTS));
        //GSM and UMTS supported, waiting for RAT change indication
        res = le_sem_WaitWithTimeOut(ThreadSemaphore, time);
        // RAT change indication not received
        LE_ASSERT(res == LE_OK);

        LE_ASSERT_OK(le_mrc_GetRatPreferences(&bitMask));
        PrintRat(bitMask);

        LE_ASSERT(LE_MRC_BITMASK_RAT_UMTS == bitMask);
    }


    if (lteSupported)
    {
        LE_INFO("Set RAT to AUTO mode");
        LE_ASSERT_OK(le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_ALL));
        // (AUTO) supported, waiting for RAT change indication if needed.
        res = le_sem_WaitWithTimeOut(ThreadSemaphore, time);
        if (LE_TIMEOUT == res)
        {
            LE_WARN("RAT change indication not received, (RAT change may noy occur)");
        }
        LE_ASSERT_OK(le_mrc_GetRatPreferences(&bitMask));
        PrintRat(bitMask);

        LE_ASSERT(LE_MRC_BITMASK_RAT_ALL == bitMask);
    }

    res = le_mrc_GetRadioAccessTechInUse(&rat);
    LE_ASSERT(res == LE_OK);

    if ((bitMaskOrigin & LE_MRC_BITMASK_RAT_CDMA) && (rat != LE_MRC_RAT_CDMA))
    {
        LE_INFO("Set RAT to CDMA");

        res = le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_CDMA);
        LE_ASSERT((LE_OK == res) || (LE_UNSUPPORTED == res));

        if (LE_OK == res)
        {
            //CDMA supported, waiting for RAT change indication
            res = le_sem_WaitWithTimeOut(ThreadSemaphore, time);
            LE_ASSERT(res == LE_OK);
            LE_ASSERT_OK(le_mrc_GetRatPreferences(&bitMask));
            PrintRat(bitMask);

            LE_ASSERT(LE_MRC_BITMASK_RAT_CDMA == bitMask);
        }
        else
        {
            LE_WARN("CDMA is not supported");
        }
    }

    // If current RAT in use is not  TDSCDMA, then set RAT to TDSCDMA
    // and test if RAT change indication is received.
    res = le_mrc_GetRadioAccessTechInUse(&rat);
    LE_ASSERT(res == LE_OK);

    if ((bitMaskOrigin & LE_MRC_BITMASK_RAT_TDSCDMA) && (rat != LE_MRC_RAT_TDSCDMA))
    {
        LE_INFO("Set RAT to TD-SCDMA");

        res = le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_TDSCDMA);
        LE_ASSERT((LE_OK == res) || (LE_UNSUPPORTED == res));

        if (LE_OK == res)
        {
            // CDMA supported, waiting for RAT change indication
            res = le_sem_WaitWithTimeOut(ThreadSemaphore, time);
            LE_ASSERT(res == LE_OK);
            LE_ASSERT_OK(le_mrc_GetRatPreferences(&bitMask));
            PrintRat(bitMask);

            LE_ASSERT(LE_MRC_BITMASK_RAT_CDMA == bitMask);
        }
        else
        {
            LE_INFO("TD-SCDMA is not supported");
        }
    }

    // If current RAT in use is not  UMTS, then set RAT to UMTS
    // and test if RAT change indication is received.
    res = le_mrc_GetRadioAccessTechInUse(&rat);
    LE_ASSERT(res == LE_OK);

    if (rat != LE_MRC_RAT_UMTS)
    {
        LE_INFO("Set RAT from %s to %s", RatTypeName[rat], RatTypeName[LE_MRC_RAT_UMTS]);
        LE_ASSERT_OK(le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_UMTS));
        //UMTS supported, waiting for RAT change indication
        res = le_sem_WaitWithTimeOut(ThreadSemaphore, time);
        LE_ASSERT(res == LE_OK);
        LE_ASSERT_OK(le_mrc_GetRatPreferences(&bitMask));
        PrintRat(bitMask);

        LE_ASSERT(LE_MRC_BITMASK_RAT_UMTS == bitMask);
    }

    //restore RAT
    LE_ASSERT_OK(le_mrc_SetRatPreferences(bitMaskOrigin));
    LE_ASSERT_OK(le_mrc_GetRatPreferences(&bitMask));
    PrintRat(bitMask);

    le_thread_Cancel(RegistrationThreadRef);
    le_mrc_RemoveRatChangeHandler(RatChangeHdlrRef);
    le_sem_Delete(ThreadSemaphore);
}
//! [RAT Preferences]

//--------------------------------------------------------------------------------------------------
/**
 * Test: PCI Network Scan.
 *
 * le_mrc_PerformPciNetworkScan() API test
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_PerformPciNetworkScan
(
    void
)
{
//! [PCI Scan]
    uint16_t physicalCellId = 0;
    uint32_t globalCellId = 0;
    char mcc[LE_MRC_MCC_BYTES] = {0};
    char mnc[LE_MRC_MNC_BYTES] = {0};
    le_mrc_PciScanInformationListRef_t scanInfoListRef = NULL;
    le_mrc_PciScanInformationRef_t     scanInfoRef     = NULL;
    le_mrc_PlmnInformationRef_t        plmnInfoRef     = NULL;

    // Request an LTE PCI scan
    scanInfoListRef = le_mrc_PerformPciNetworkScan(LE_MRC_BITMASK_RAT_LTE);
    LE_ASSERT(scanInfoListRef != NULL);

    // Get reference to first cell info
    scanInfoRef = le_mrc_GetFirstPciScanInfo(scanInfoListRef);
    LE_ASSERT(scanInfoRef != NULL);

    do
    {
        physicalCellId = le_mrc_GetPciScanCellId(scanInfoRef);
        globalCellId = le_mrc_GetPciScanGlobalCellId(scanInfoRef);

        // Get reference to the first PLMN info
        plmnInfoRef = le_mrc_GetFirstPlmnInfo(scanInfoRef);
        LE_ASSERT(plmnInfoRef != NULL);

        do
        {
            if (LE_OK == le_mrc_GetPciScanMccMnc(plmnInfoRef,
                                                 mcc,
                                                 LE_MRC_MCC_BYTES,
                                                 mnc,
                                                 LE_MRC_MCC_BYTES))
            {
                LE_INFO("Cell ID: physical %"PRIu16" global %"PRIu32", MCC: %s, MNC: %s",
                        physicalCellId, globalCellId, mcc, mnc);
            }

            plmnInfoRef = le_mrc_GetNextPlmnInfo(scanInfoRef);

        }
        while (plmnInfoRef);

        scanInfoRef = le_mrc_GetNextPciScanInfo(scanInfoListRef);

    }
    while (scanInfoRef);

    le_mrc_DeletePciNetworkScan(scanInfoListRef);
//! [PCI Scan]
}


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

    res = le_mrc_GetCellularNetworkMccMnc(scanInfoRef,
                                          mcc,
                                          LE_MRC_MCC_BYTES,
                                          mnc,
                                          LE_MRC_MNC_BYTES);
    LE_ASSERT(res == LE_OK);

    res = le_mrc_GetCellularNetworkName(scanInfoRef, nameStr, 1);
    LE_ASSERT(res == LE_OVERFLOW);
    res = le_mrc_GetCellularNetworkName(scanInfoRef, nameStr, 100);;
    LE_ASSERT(res == LE_OK);
    LE_INFO("cellular network name.%s  (mcc=%s, mnc=%s)", nameStr, mcc, mnc);

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
static void Testle_mrc_PerformCellularNetworkScan
(
    void
)
{
    le_result_t res;
    le_mrc_RatBitMask_t bitMaskOrigin = 0;
    le_mrc_ScanInformationListRef_t scanInfoListRef = NULL;
    le_mrc_ScanInformationRef_t     scanInfoRef = NULL;

    // Get the current rat preference.
    res = le_mrc_GetRatPreferences(&bitMaskOrigin);
    LE_ASSERT(res == LE_OK);

    scanInfoListRef = le_mrc_PerformCellularNetworkScan(bitMaskOrigin);
    if (scanInfoListRef != NULL)
    {
        scanInfoRef = le_mrc_GetFirstCellularNetworkScan(scanInfoListRef);
        LE_ASSERT(scanInfoRef != NULL);
        ReadScanInfo(scanInfoRef);

        while ((scanInfoRef = le_mrc_GetNextCellularNetworkScan(scanInfoListRef)) != NULL)
        {
            ReadScanInfo(scanInfoRef);
        }

        le_mrc_DeleteCellularNetworkScan(scanInfoListRef);
    }
    res = le_mrc_SetRatPreferences(bitMaskOrigin);
    LE_ASSERT(LE_OK == res);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Cellular Network Scan handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyNetworkPciScanHandler
(
    le_mrc_PciScanInformationListRef_t listRef,
    void* contextPtr
)
{
    le_result_t res;
    char mcc[LE_MRC_MCC_BYTES] = {0};
    char mnc[LE_MRC_MNC_BYTES] = {0};
    le_mrc_PciScanInformationRef_t     scanInfoRef = NULL;
    le_mrc_PlmnInformationRef_t   PlmnInfoRef = NULL;

    if (NULL != listRef)
    {
        scanInfoRef = le_mrc_GetFirstPciScanInfo(listRef);
        LE_ASSERT(scanInfoRef != NULL);
        uint16_t physicalCellId = le_mrc_GetPciScanCellId(scanInfoRef);
        uint32_t globalCellId = le_mrc_GetPciScanGlobalCellId(scanInfoRef);
        LE_INFO("First cell ID: physical %"PRIu16" global %"PRIu32"", physicalCellId, globalCellId);
        //Get first plmninfo reference
        PlmnInfoRef = le_mrc_GetFirstPlmnInfo(scanInfoRef);
        if(PlmnInfoRef == NULL)
        {
            LE_INFO("FAIL to get ref to plmn info");
        }
        else
        {
            LE_INFO(" SUCCESS to get ref to plmn info");
            res = le_mrc_GetPciScanMccMnc(PlmnInfoRef,
                                          mcc,
                                          LE_MRC_MCC_BYTES,
                                          mnc,
                                          LE_MRC_MNC_BYTES);
            if (res == LE_OK )
            {
               LE_INFO("the first value of mcc in the first cell is :%s",mcc);
               LE_INFO("the first value of mnc in the first cell is :%s",mnc);
            }
        }
        while ((PlmnInfoRef = le_mrc_GetNextPlmnInfo(scanInfoRef)) != NULL)
        {
            res = le_mrc_GetPciScanMccMnc(PlmnInfoRef,
                                          mcc,
                                          LE_MRC_MCC_BYTES,
                                          mnc,
                                          LE_MRC_MNC_BYTES);
            if (res == LE_OK )
            {
                LE_INFO("The Next value of mcc in the first cell is :%s",mcc);
                LE_INFO("The Next value of mnc in the first cell is :%s",mnc);
            }
        }

        while ((scanInfoRef = le_mrc_GetNextPciScanInfo(listRef)) != NULL)
            {
                physicalCellId = le_mrc_GetPciScanCellId(scanInfoRef);
                globalCellId = le_mrc_GetPciScanGlobalCellId(scanInfoRef);
                LE_INFO("Next cell ID: physical %"PRIu16" global %"PRIu32"", physicalCellId,
                        globalCellId);
                PlmnInfoRef = le_mrc_GetFirstPlmnInfo(scanInfoRef);

                if(PlmnInfoRef == NULL)
                {
                    LE_INFO("FAIL to get ref to plmn info");
                }
                else
                {
                    LE_INFO(" SUCCESS to get ref to plmn info");
                    res = le_mrc_GetPciScanMccMnc(PlmnInfoRef,
                                                  mcc,
                                                  LE_MRC_MCC_BYTES,
                                                  mnc,
                                                  LE_MRC_MNC_BYTES);
                    if (res == LE_OK )
                    {
                    LE_INFO("the first value of mcc in the next cell is :%s",mcc);
                    LE_INFO("the first value of mnc in the next cell is :%s",mnc);
                    }
                }

                while ((PlmnInfoRef = le_mrc_GetNextPlmnInfo(scanInfoRef)) != NULL)
                {
                    res = le_mrc_GetPciScanMccMnc(PlmnInfoRef,
                                                  mcc,
                                                  LE_MRC_MCC_BYTES,
                                                  mnc,
                                                  LE_MRC_MNC_BYTES);
                    if (res == LE_OK )
                    {
                        LE_INFO("The Next value of mcc in the Next cell is :%s",mcc);
                        LE_INFO("The Next value of mnc in the Next cell is :%s",mnc);
                    }
                }
            }
   }
    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread for asynchronous Pci scan test.
 *
 * Test API: le_mrc_PerformPciNetworkScanAsync() API test
 */
//--------------------------------------------------------------------------------------------------
static void* MyNetworkPciScanAsyncThread
(
    void* context   ///< See parameter documentation above.
)
{
    le_mrc_RatBitMask_t bitMaskOrigin;

    le_mrc_ConnectService();

    // Get the current rat preference.
    le_result_t res = le_mrc_GetRatPreferences(&bitMaskOrigin);
    LE_ASSERT(res == LE_OK);

    le_mrc_PerformPciNetworkScanAsync(bitMaskOrigin, MyNetworkPciScanHandler, NULL);
    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Cellular Network Scan asynchronous.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_PerformPciNetworkScanAsync
(
    void
)
{
    le_result_t res;
    le_clk_Time_t time = {0,0};
    le_mrc_RatBitMask_t bitMaskOrigin = 0;
    time.sec = 120000;

    res = le_mrc_GetRatPreferences(&bitMaskOrigin);
    LE_ASSERT(LE_OK == res);

    // Init the semaphore for asynchronous callback
    ThreadSemaphore = le_sem_Create("HandlerSem",0);

    RegistrationThreadRef = le_thread_Create("CallBack", MyNetworkPciScanAsyncThread, NULL);
    le_thread_Start(RegistrationThreadRef);

    // Wait for complete asynchronous registration
    res = le_sem_WaitWithTimeOut(ThreadSemaphore, time);
    LE_ERROR_IF(res != LE_OK, "SYNC FAILED");
    le_thread_Cancel(RegistrationThreadRef);

    le_sem_Delete(ThreadSemaphore);

    le_mrc_SetRatPreferences(bitMaskOrigin);

    sleep(5);
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

    if (NULL != listRef)
    {
        scanInfoRef = le_mrc_GetFirstCellularNetworkScan(listRef);
        LE_ASSERT(scanInfoRef != NULL);

        if (scanInfoRef != NULL)
        {
            ReadScanInfo(scanInfoRef);

            while ((scanInfoRef = le_mrc_GetNextCellularNetworkScan(listRef)) != NULL)
            {
                ReadScanInfo(scanInfoRef);
            }
            le_mrc_DeleteCellularNetworkScan(listRef);
        }
    }
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

    le_mrc_PerformCellularNetworkScanAsync(bitMaskOrigin, MyNetworkScanHandler, NULL);
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
static void Testle_mrc_PerformCellularNetworkScanAsync
(
    void
)
{
    le_result_t res;
    le_clk_Time_t time = {0,0};
    le_mrc_RatBitMask_t bitMaskOrigin = 0;
    time.sec = 120000;

    res = le_mrc_GetRatPreferences(&bitMaskOrigin);
    LE_ASSERT(LE_OK == res);

    // Init the semaphore for asynchronous callback
    ThreadSemaphore = le_sem_Create("HandlerSem",0);

    RegistrationThreadRef = le_thread_Create("CallBack", MyNetworkScanAsyncThread, NULL);
    le_thread_Start(RegistrationThreadRef);

    // Wait for complete asynchronous registration
    res = le_sem_WaitWithTimeOut(ThreadSemaphore, time);
    LE_ERROR_IF(res != LE_OK, "SYNC FAILED");
    le_thread_Cancel(RegistrationThreadRef);

    le_sem_Delete(ThreadSemaphore);

    le_mrc_SetRatPreferences(bitMaskOrigin);

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
static void Testle_mrc_BandPreferences
(
    void
)
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
static void Testle_mrc_LteBandPreferences
(
    void
)
{
    le_result_t res;
    le_mrc_LteBandBitMask_t bandMask = 0;
    le_mrc_LteBandBitMask_t bandMaskOrigin = 0;

#define  PRINT_BANDLTE(x)   LE_INFO("LTE Band MRC preferences 0x%016"PRIX64, x);

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
static void Testle_mrc_TdScdmaBandPreferences
(
    void
)
{
    le_result_t res;
    le_mrc_TdScdmaBandBitMask_t bandMask = 0;
    le_mrc_TdScdmaBandBitMask_t bandMaskOrigin = 0;

#define  PRINT_BANDCDMA(x)   LE_INFO("TD-SCDMA Band preferences 0x%016X", x);

    // Get the current rat preference.
    res = le_mrc_GetTdScdmaBandPreferences(&bandMaskOrigin);
    LE_ASSERT((LE_OK == res) || (LE_UNSUPPORTED == res));

    if (LE_OK == res)
    {
        // The TD-SCDMA is supported
        LE_ASSERT(0 != bandMaskOrigin);
        PRINT_BANDCDMA(bandMaskOrigin);
        LE_ASSERT_OK(le_mrc_SetTdScdmaBandPreferences(bandMaskOrigin));

        // Get the current rat preference.
        LE_ASSERT_OK(le_mrc_GetTdScdmaBandPreferences(&bandMask));
        PRINT_BANDCDMA(bandMask);
        LE_ASSERT(bandMask == bandMaskOrigin);
    }
    else
    {
        // Try to set a Band when TD-SCDMA is not supported
        LE_ASSERT(LE_UNSUPPORTED == le_mrc_SetTdScdmaBandPreferences(
                                                       LE_MRC_BITMASK_TDSCDMA_BAND_A |
                                                       LE_MRC_BITMASK_TDSCDMA_BAND_F));

        // Try to get the TD-SCDMA Band Preferences when TD-SCDMA is not supported
        LE_ASSERT(LE_UNSUPPORTED == le_mrc_GetTdScdmaBandPreferences(&bandMask));
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
static void Testle_mrc_GetStateAndQual
(
    void
)
{
    le_result_t           res;
    le_mrc_NetRegState_t  state;
    uint32_t              quality;

    res = le_mrc_GetNetRegState(&state);
    LE_ASSERT(res == LE_OK);
    LE_ASSERT((state>=LE_MRC_REG_NONE) && (state<=LE_MRC_REG_UNKNOWN));

    res = le_mrc_GetSignalQual(&quality);
    LE_ASSERT(res == LE_OK);
    LE_ASSERT(quality<=5);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Get Signal Metrics.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_GetSignalMetrics
(
    void
)
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
        case LE_MRC_RAT_TDSCDMA:
            res = le_mrc_GetUmtsSignalMetrics(metricsRef, &rxLevel, &er, &ecio, &rscp, &sinr);
            LE_ASSERT(res == LE_OK);
            LE_INFO("UMTS/TD-SCDMA metrics rxLevel.%ddBm, er.%d, ecio.%010.1fdB,"
                    "rscp.%ddBm, sinr.%ddB", rxLevel, er, ((double)ecio/10), rscp, sinr);
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
static void Testle_mrc_GetNeighboringCellsInfo
(
    void
)
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
 * Handler function for Signal Strength change Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestSsHandler
(
    int32_t     ss,
    void*       contextPtr
)
{
    LE_INFO("New Signal Strength change: %ddBm", ss);
    le_sem_Post(ThreadSemaphore);
}


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
 * Thread for test Signal Strength indication.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* MySignalStrengthThread
(
    void* context   ///< Context
)
{
    le_sim_ConnectService();
    le_mrc_ConnectService();

    le_mrc_Rat_t rat = *((le_mrc_Rat_t *) context);

    LE_INFO("Set Signal handler on rat %d", rat);
    SignalHdlrRef = le_mrc_AddSignalStrengthChangeHandler(rat,
                                                          -110,
                                                          -100,
                                                          TestSsHandler,
                                                          NULL);

    LE_ASSERT(SignalHdlrRef);

    le_sem_Post(ThreadSemaphore);

    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Signal Strength change handling.
 *
 * Test API: le_mrc_SetSignalStrengthIndThresholds() API test
 **/
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_SetSignalStrengthIndThresholds
(
    void
)
{
    le_mrc_Rat_t rat;
    le_result_t res;
    int32_t ss = 0;
    int32_t ecio, rscp, sinr, io;
    uint32_t ber, bler, er;
    le_clk_Time_t time1;
    time1.sec = 150;
    time1.usec =0;

    sleep(SLEEP_5S);

    LE_ASSERT_OK(le_mrc_GetRadioAccessTechInUse(&rat));
    LE_ASSERT(LE_MRC_RAT_UNKNOWN != rat);

    // Init the semaphore for asynchronous callback
    ThreadSemaphore = le_sem_Create("HandlerSignalStrength", 0);

    SignalStrengthChangeThreadRef = le_thread_Create("ThreadStrengthInd", MySignalStrengthThread,
        (void *) &rat);
    le_thread_Start(SignalStrengthChangeThreadRef);

    // Wait for complete asynchronous registration
    res = le_sem_WaitWithTimeOut(ThreadSemaphore, time1);
    LE_ASSERT_OK(res);

    le_mrc_MetricsRef_t metrics = le_mrc_MeasureSignalMetrics();
    LE_ASSERT(metrics);

    switch(rat)
    {
        case LE_MRC_RAT_GSM:
            res = le_mrc_GetGsmSignalMetrics(metrics, &ss, &ber);
            break;

        case LE_MRC_RAT_UMTS:
        case LE_MRC_RAT_TDSCDMA:
            res = le_mrc_GetUmtsSignalMetrics(metrics, &ss, &bler, &ecio, &rscp, &sinr);
            break;

        case LE_MRC_RAT_LTE:
            res = le_mrc_GetLteSignalMetrics(metrics, &ss, &bler, &rscp, &rscp, &sinr);
            break;

        case LE_MRC_RAT_CDMA:
            res = le_mrc_GetCdmaSignalMetrics(metrics, &ss, &er, &ecio, &sinr, &io);
            break;

        default:
            res = LE_FAULT;
            LE_ERROR("Unknow RAT");
            break;
    }
    le_mrc_DeleteSignalMetrics(metrics);

    LE_ASSERT_OK(res);

    LE_INFO("Signal %d, rat %d", ss, rat);

    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndThresholds(rat, ss-1, ss+1));

    // Wait for complete asynchronous registration
    res = le_sem_WaitWithTimeOut(ThreadSemaphore, time1);
    LE_ASSERT_OK(res);

    le_mrc_RemoveSignalStrengthChangeHandler(SignalHdlrRef);

    le_thread_Cancel(SignalStrengthChangeThreadRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Signal Strength change handling.
 *
 * Test API: le_mrc_SetSignalStrengthIndDelta() API test
 **/
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_SetSignalStrengthIndDelta
(
    void
)
{
    le_mrc_Rat_t rat;
    le_result_t res;
    le_mrc_NetRegState_t  state;
    int32_t ss = 0;
    int32_t ecio, rscp, sinr, io;
    uint32_t ber, bler, er;
    le_clk_Time_t time1;
    time1.sec = 150;
    time1.usec =0;

    // test bad parameters
    LE_ASSERT(LE_BAD_PARAMETER == le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_UNKNOWN,2));
    LE_ASSERT(LE_BAD_PARAMETER == le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_UNKNOWN,0));
    LE_ASSERT(LE_BAD_PARAMETER == le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_GSM,0));
    LE_ASSERT(LE_BAD_PARAMETER == le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_TDSCDMA,1));
    LE_ASSERT(LE_BAD_PARAMETER == le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_TDSCDMA,9));

    // test correct parameters.
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_GSM, 1));
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_CDMA,10));
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_CDMA,62));
    // There is no max value testing in Legato although the max practical value should be less than
    // RSSI_MAX - RSSI_MIN.
    // #define RSSI_MIN        51   /* per 3GPP 27.007  (negative value) */
    // #define RSSI_MAX        113  /* per 3GPP 27.007  (negative value) */
    // It is up to user to set a reasonable delta.
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_LTE,630));
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_UMTS,1000));

    // TD-SCDMA tests.
    // set 1 dBm RSSI delta
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_TDSCDMA,10));
    // set 1 dBm RSSI delta
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_TDSCDMA,14));
    // set 2 dBm RSSI delta
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_TDSCDMA,16));
    // set 9 dBm RSSI delta
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_TDSCDMA,95));
    // set 10 dBm RSSI delta
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_TDSCDMA,101));
    // set 19 dBm RSSI delta
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_TDSCDMA,195));
    // set 20 dBm RSSI delta
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_TDSCDMA,196));

    LE_ASSERT_OK(le_mrc_GetNetRegState(&state));
    LE_ASSERT_OK(le_mrc_GetRadioAccessTechInUse(&rat));

    LE_ASSERT((LE_MRC_RAT_UNKNOWN != rat) &&
              ((LE_MRC_REG_HOME == state) || (LE_MRC_REG_ROAMING == state))
             )
    // Init the semaphore for asynchronous callback
    ThreadSemaphore = le_sem_Create("HandlerSignalStrength", 0);

    SignalStrengthChangeThreadRef = le_thread_Create("ThreadStrengthInd", MySignalStrengthThread,
        (void *) &rat);
    // Join the thread
    le_thread_SetJoinable(SignalStrengthChangeThreadRef);

    le_thread_Start(SignalStrengthChangeThreadRef);

    // Wait for complete asynchronous registration
    res = le_sem_WaitWithTimeOut(ThreadSemaphore, time1);
    LE_ASSERT_OK(res);

    le_mrc_MetricsRef_t metrics = le_mrc_MeasureSignalMetrics();
    LE_ASSERT(metrics);

    switch(rat)
    {
        case LE_MRC_RAT_GSM:
            res = le_mrc_GetGsmSignalMetrics(metrics, &ss, &ber);
            break;

        case LE_MRC_RAT_UMTS:
        case LE_MRC_RAT_TDSCDMA:
            res = le_mrc_GetUmtsSignalMetrics(metrics, &ss, &bler, &ecio, &rscp, &sinr);
            break;

        case LE_MRC_RAT_LTE:
            res = le_mrc_GetLteSignalMetrics(metrics, &ss, &bler, &rscp, &rscp, &sinr);
            break;

        case LE_MRC_RAT_CDMA:
            res = le_mrc_GetCdmaSignalMetrics(metrics, &ss, &er, &ecio, &sinr, &io);
            break;

        default:
            res = LE_FAULT;
            LE_ERROR("Unknow RAT");
            break;
    }
    le_mrc_DeleteSignalMetrics(metrics);

    LE_ASSERT_OK(res);

    LE_INFO("Signal %d, rat %d", ss, rat);

    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndDelta(rat, 1));

    // Wait for signal event
    res = le_sem_WaitWithTimeOut(ThreadSemaphore, time1);
    LE_ASSERT_OK(res);

    le_mrc_RemoveSignalStrengthChangeHandler(SignalHdlrRef);

    LE_ASSERT_OK(le_thread_Cancel(SignalStrengthChangeThreadRef));
    LE_ASSERT_OK(le_thread_Join(SignalStrengthChangeThreadRef,NULL));
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Signal Strength change handling.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_SsHdlr
(
    void
)
{
    le_mrc_SignalStrengthChangeHandlerRef_t testHdlrRef1 = NULL;
    le_mrc_SignalStrengthChangeHandlerRef_t testHdlrRef2 = NULL;
    le_mrc_SignalStrengthChangeHandlerRef_t testHdlrRef3 = NULL;
    le_mrc_SignalStrengthChangeHandlerRef_t testHdlrRef4 = NULL;

    LE_INFO("Set Signal handler with lowerRangeThreshold out of range to be rejected by PA QMI");
    testHdlrRef1 = le_mrc_AddSignalStrengthChangeHandler(LE_MRC_RAT_GSM,
                                                        -40000,
                                                        -32000,
                                                        TestSsHandler,
                                                        NULL);
    LE_ASSERT(NULL == testHdlrRef1);

    testHdlrRef1 = le_mrc_AddSignalStrengthChangeHandler(LE_MRC_RAT_GSM,
                                                        -80,
                                                        -70,
                                                        TestGsmSsHandler,
                                                        NULL);
    LE_ASSERT(testHdlrRef1);

    testHdlrRef2 = le_mrc_AddSignalStrengthChangeHandler(LE_MRC_RAT_UMTS,
                                                        -200,
                                                        -70,
                                                        TestUmtsSsHandler,
                                                        NULL);
    LE_ASSERT(testHdlrRef2);

    testHdlrRef3 = le_mrc_AddSignalStrengthChangeHandler(LE_MRC_RAT_LTE,
                                                        -80,
                                                        0,
                                                        TestLteSsHandler,
                                                        NULL);
    LE_ASSERT(testHdlrRef3);

    testHdlrRef4 = le_mrc_AddSignalStrengthChangeHandler(LE_MRC_RAT_CDMA,
                                                        -80,
                                                        10,
                                                        TestCdmaSsHandler,
                                                        NULL);
    LE_ASSERT(testHdlrRef4);

    le_mrc_RemoveSignalStrengthChangeHandler(testHdlrRef1);
    le_mrc_RemoveSignalStrengthChangeHandler(testHdlrRef2);
    le_mrc_RemoveSignalStrengthChangeHandler(testHdlrRef3);
    le_mrc_RemoveSignalStrengthChangeHandler(testHdlrRef4);
}


//! [Loc information]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Location information.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_GetLocInfo
(
    void
)
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
static void Testle_mrc_PreferredPLMN
(
    void
)
{
    le_result_t res;
    char mccStr[LE_MRC_MCC_BYTES] = {0};
    char mncStr[LE_MRC_MNC_BYTES] = {0};
    char saveMccStr[3][LE_MRC_MCC_BYTES] = {{0}};
    char saveMncStr[3][LE_MRC_MNC_BYTES] = {{0}};
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
            //MCC string length must be 3
            res = le_mrc_GetPreferredOperatorDetails(optRef,
                mccStr, LE_MRC_MCC_BYTES-1,
                mncStr, LE_MRC_MNC_BYTES,
                &ratMask);
            LE_ASSERT(res == LE_OVERFLOW);

            //MNC string length can be 2 or 3
            res = le_mrc_GetPreferredOperatorDetails(optRef,
                mccStr, LE_MRC_MCC_BYTES,
                mncStr, LE_MRC_MNC_BYTES-2,
                &ratMask);
            LE_ASSERT(res == LE_OVERFLOW);

            //Retrieve MCC/MNC for further saving to buffer
            res = le_mrc_GetPreferredOperatorDetails(optRef,
                mccStr, LE_MRC_MCC_BYTES,
                mncStr, LE_MRC_MNC_BYTES,
                &ratMask);
            LE_ASSERT(res == LE_OK);

            if(beforeIndex < 3)
            {
                strncpy(saveMccStr[beforeIndex], mccStr, LE_MRC_MCC_BYTES - 1);
                strncpy(saveMncStr[beforeIndex], mncStr, LE_MRC_MNC_BYTES - 1);
                saveMccStr[beforeIndex][LE_MRC_MCC_BYTES - 1] = '\0';
                saveMncStr[beforeIndex][LE_MRC_MNC_BYTES - 1] = '\0';


                saveRat[beforeIndex] = ratMask;
                LE_INFO("Save (%d) mcc=%s mnc=%s rat=%d", beforeIndex,
                    saveMccStr[beforeIndex],
                    saveMncStr[beforeIndex],
                    saveRat[beforeIndex] );
            }
            else
            {
                LE_INFO("Get_detail Loop(%d) mcc.%s mnc %s, rat.%08X,"
                         "GSM %c, LTE %c, UMTS %c, TD-SCDMA %c, ALL %c",
                    beforeIndex, mccStr, mncStr, ratMask,
                    (ratMask & LE_MRC_BITMASK_RAT_GSM ? 'Y':'N'),
                    (ratMask & LE_MRC_BITMASK_RAT_LTE ? 'Y':'N'),
                    (ratMask & LE_MRC_BITMASK_RAT_UMTS ? 'Y':'N'),
                    (ratMask & LE_MRC_BITMASK_RAT_TDSCDMA ? 'Y':'N'),
                    (ratMask & LE_MRC_BITMASK_RAT_ALL ? 'Y':'N')
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
    LE_ASSERT(res == LE_NOT_FOUND);
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
        LE_INFO("Get_detail Loop(%d) mcc.%s mnc %s, rat.%08X,"
                 "GSM %c, LTE %c, UMTS %c, TD-SCDMA %c, ALL %c",
            ++afterIndex, mccStr, mncStr, ratMask,
            (ratMask & LE_MRC_BITMASK_RAT_GSM ? 'Y':'N'),
            (ratMask & LE_MRC_BITMASK_RAT_LTE ? 'Y':'N'),
            (ratMask & LE_MRC_BITMASK_RAT_UMTS ? 'Y':'N'),
            (ratMask & LE_MRC_BITMASK_RAT_TDSCDMA ? 'Y':'N'),
            (ratMask & LE_MRC_BITMASK_RAT_ALL ? 'Y':'N')
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
        res = le_mrc_AddPreferredOperator(saveMccStr[2], saveMncStr[2], saveRat[2]);
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
//--------------------------------------------------------------------------------------------------
/**
 * Test band capabilities according to RAT
 */
//--------------------------------------------------------------------------------------------------
static bool Test_BandCapabilities
(
    le_mrc_RatBitMask_t ratMask,
    le_result_t res
)
{
    bool test = true;
    le_mrc_RatBitMask_t bitMaskOrigin;

    // Get the current rat preference.
    LE_ASSERT_OK(le_mrc_GetRatPreferences(&bitMaskOrigin));

    if ((LE_UNSUPPORTED == le_mrc_SetRatPreferences(ratMask)) && (LE_UNSUPPORTED != res))
    {
        test = false;
    }
    // Restore origin RAT.
    LE_ASSERT_OK(le_mrc_SetRatPreferences(bitMaskOrigin));
    return test;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Get platform band capabilities.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_GetBandCapabilities
(
    void
)
{
    le_mrc_BandBitMask_t        bands;
    le_mrc_LteBandBitMask_t     lteBands;
    le_mrc_TdScdmaBandBitMask_t tdScdmaBands;
    le_result_t                 res;

    res = le_mrc_GetBandCapabilities(&bands);
    LE_ASSERT(LE_FAULT != res);
    // Not supported on all platform
    switch(res)
    {
        case LE_OK:
            LE_ASSERT(0L != bands);
            LE_INFO("Get 2G/3G Band Capabilities bit mask: 0x%016"PRIX64, (uint64_t)bands);
            break;
        case LE_UNSUPPORTED:
            LE_WARN("Unable to get Band Capabilities on this platform");
            break;
        default:
            LE_FATAL("le_mrc_GetBandCapabilities failed");
            break;
    }

    // Test if LTE Band Capabilities are supported
    res = le_mrc_GetLteBandCapabilities(&lteBands);
    LE_ASSERT(LE_FAULT != res);

    LE_ASSERT(true == Test_BandCapabilities(LE_MRC_BITMASK_RAT_LTE, res));

    switch(res)
    {
        case LE_OK:
            LE_ASSERT(0 != lteBands);
            LE_INFO("Get LTE Band Capabilities bit mask: 0x%016"PRIX64, (uint64_t)lteBands);
            break;
        case LE_UNSUPPORTED:
            LE_WARN("Unable to get Band Capabilities on this platform");
            break;
        default:
            LE_FATAL("le_mrc_GetLteBandCapabilities failed");
            break;
    }

    // Test if TD-SCDMA Band Capabilities are supported
    res = le_mrc_GetTdScdmaBandCapabilities(&tdScdmaBands);
    LE_ASSERT(LE_FAULT != res);

    LE_ASSERT(true == Test_BandCapabilities(LE_MRC_BITMASK_RAT_TDSCDMA, res));

    switch(res)
    {
        case LE_OK:
            LE_ASSERT(0 != tdScdmaBands);
            LE_INFO("Get TD-SCDMA Band Capabilities bit mask: 0x%016"PRIX64,
                    (uint64_t)tdScdmaBands);
            break;
        case LE_UNSUPPORTED:
            LE_WARN("Unable to get Band Capabilities on this platform");
            break;
        default:
            LE_FATAL("le_mrc_GetTdScdmaBandCapabilities failed");
            break;
    }
}
//! [Band Capabilities]

//! [Jamming detection]
//--------------------------------------------------------------------------------------------------
/**
 * Test: Jamming detection.
 *
 * le_mrc_StartJammingDetection() API test
 * le_mrc_StopJammingDetection() API test
 * le_mrc_AddJammingDetectionEventHandler() API test
 * le_mrc_RemoveJammingDetectionEventHandler() API test
 */
//--------------------------------------------------------------------------------------------------
static le_mrc_JammingDetectionEventHandlerRef_t testJammingHdlrRef = NULL;

//-------------------------------------------------------------------------------------------------
/**
 * Jamming detection event handler
 */
//-------------------------------------------------------------------------------------------------
static void TestJammingHandler
(
    le_mrc_JammingReport_t  report,     ///< [IN] Report type.
    le_mrc_JammingStatus_t  status,     ///< [IN] Jamming detection status.
    void*                   contextPtr  ///< [IN] Handler context.
)
{
    LE_DEBUG("Jamming detection notification: report %d, status %d", report, status);
    if (LE_MRC_JAMMING_REPORT_FINAL == report)
    {
        le_mrc_RemoveJammingDetectionEventHandler(testJammingHdlrRef);
        testJammingHdlrRef = NULL;
        LE_ASSERT_OK(le_mrc_StopJammingDetection());
        le_sem_Post(ThreadSemaphore);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread for Jamming detection.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* MyJammingDetectionThread
(
    void* context   ///< Context
)
{
    le_result_t res;
    le_mrc_ConnectService();

    res = le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_LTE);
    LE_ASSERT((LE_UNSUPPORTED == res) || (LE_OK == res));

    if (LE_UNSUPPORTED == res)
    {
        le_sem_Post(ThreadSemaphore);
        return NULL;
    }

    sleep(SLEEP_5S);

    testJammingHdlrRef = le_mrc_AddJammingDetectionEventHandler(TestJammingHandler, NULL);
    LE_ASSERT(testJammingHdlrRef);

    res = le_mrc_StartJammingDetection();
    if (LE_UNSUPPORTED == res)
    {
        LE_INFO("Unsupported JammingDetection, remove EventHandler");
        le_mrc_RemoveJammingDetectionEventHandler(testJammingHdlrRef);
        testJammingHdlrRef = NULL;
        le_sem_Post(ThreadSemaphore);
    }
    else
    {
        LE_ASSERT_OK(res);
        LE_ASSERT_OK(le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_GSM));

        le_event_RunLoop();
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Jamming detection.
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_JammingDetection
(
    void
)
{
    le_mrc_RatBitMask_t bitMaskOrigin;
    le_clk_Time_t time1;
    le_result_t res;

    sleep(SLEEP_5S);

    time1.sec = 150;
    time1.usec = 0;

    LE_ASSERT_OK(le_mrc_GetRatPreferences(&bitMaskOrigin));
    PrintRat(bitMaskOrigin);

    // Init the semaphore for asynchronous callback
    ThreadSemaphore = le_sem_Create("JammingDetection", 0);

    JammingDetectionTreadRef = le_thread_Create("ThreadJammingInd", MyJammingDetectionThread, NULL);
    le_thread_Start(JammingDetectionTreadRef);

    // Wait for complete asynchronous registration
    res = le_sem_WaitWithTimeOut(ThreadSemaphore, time1);
    LE_ASSERT_OK(res);

    le_thread_Cancel(JammingDetectionTreadRef);
    le_sem_Delete(ThreadSemaphore);

    LE_ASSERT_OK(le_mrc_SetRatPreferences(bitMaskOrigin));
}
//! [Jamming detection]

//--------------------------------------------------------------------------------------------------
/**
 * Test: SAR backoff setting
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_SarBackoff
(
    void
)
{
    uint8_t state;
    uint8_t i;
    le_result_t res;

    for (i = SAR_BACKOFF_STATE_DEFAULT; i <= SAR_BACKOFF_STATE_MAX; i++)
    {
        LE_ASSERT_OK(le_mrc_SetSarBackoffState(i));
        LE_ASSERT_OK(le_mrc_GetSarBackoffState(&state));
        LE_ASSERT(i == state);
        LE_INFO("Backoff state: %d", state);
    }

    res = le_mrc_SetSarBackoffState(SAR_BACKOFF_STATE_MAX+1);
    LE_ASSERT((LE_OUT_OF_RANGE == res) || (LE_FAULT == res));

    LE_ASSERT_OK(le_mrc_SetSarBackoffState(SAR_BACKOFF_STATE_DEFAULT));
}
//! [SAR backoff setting]

//--------------------------------------------------------------------------------------------------
/**
 * Main
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("======== Start MRC Modem Services implementation Test========");

    LE_INFO("======== SAR backoff setting Test ========");
    Testle_mrc_SarBackoff();
    LE_INFO("======== SAR backoff setting Test PASSED ========");


    LE_INFO("======== PerformPciNetworkScan Test ========");
    Testle_mrc_PerformPciNetworkScan();
    LE_INFO("======== PerformPciNetworkScan Test PASSED ========");

    LE_INFO("======== PerformPciNetworkScanasync Test ========");
    Testle_mrc_PerformPciNetworkScanAsync();
    LE_INFO("======== PerformPciNetworkScanasync Test PASSED ========");

    LE_INFO("======== PerformCellularNetworkScan Test ========");
    Testle_mrc_PerformCellularNetworkScan();
    LE_INFO("======== PerformCellularNetworkScan Test PASSED ========");

    LE_INFO("======== Jamming detection Test ========");
    Testle_mrc_JammingDetection();
    LE_INFO("======== Jamming detection Test PASSED ========");

    LE_INFO("======== PSHdlr Test ========");
    Testle_mrc_PSHdlr();
    LE_INFO("======== PSHdlr Test PASSED ========");

    LE_INFO("======== PreferredPLMN Test ========");
    Testle_mrc_PreferredPLMN();
    LE_INFO("======== PreferredPLMN Test PASSED ========");

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

    LE_INFO("======== Set Signal Strength Thresholds Test ========");
    Testle_mrc_SetSignalStrengthIndThresholds();
    LE_INFO("======== Set Signal Strength Thresholds Test PASSED ========");

    LE_INFO("======== Set Signal Strength delta Test ========");
    Testle_mrc_SetSignalStrengthIndDelta();
    LE_INFO("======== Set Signal Strength delta Test PASSED ========");

    LE_INFO("======== RatPreferences Test ========");
    Testle_mrc_RatPreferences();
    LE_INFO("======== RatPreferences Test PASSED ========");

    LE_INFO("======== RegisterMode Test ========");
    Testle_mrc_RegisterMode();
    LE_INFO("======== RegisterMode Test PASSED ========");

    LE_INFO("======== RegisterModeAsync Test ========");
    Testle_mrc_RegisterModeAsync();
    LE_INFO("======== RegisterModeAsync Test PASSED ========");

    LE_INFO("======== BandCapabilities Test ========");
    Testle_mrc_GetBandCapabilities();
    LE_INFO("======== BandCapabilities Test PASSED ========");

    LE_INFO("======== Test MRC Modem Services implementation Test SUCCESS ========");

    exit(EXIT_SUCCESS);
}


