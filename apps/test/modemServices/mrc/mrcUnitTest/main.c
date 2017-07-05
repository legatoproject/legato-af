/**
 * This module implements the unit tests for MRC API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "interfaces.h"
#include "log.h"
#include "pa_simu.h"
#include "pa_sim_simu.h"
#include "pa_mrc_simu.h"

#include "le_sim_local.h"
#include "le_mrc_local.h"

#define PIN_CODE "0000"
#define IMSI "208011700352758"
#define ICCID "89330123164011144830"
#define MCC "208"
#define MNC "01"
#define OPERATOR "orange"

//--------------------------------------------------------------------------------------------------
/**
 * Server Service Reference
 */
//--------------------------------------------------------------------------------------------------
static le_msg_ServiceRef_t _ServerServiceRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Client Session Reference for the current message received from a client
 */
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t _ClientSessionRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_mrc_GetServiceRef
(
    void
)
{
    return _ServerServiceRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_mrc_GetClientSessionRef
(
    void
)
{
    return _ClientSessionRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.  (STUBBED FUNCTION)

 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t le_msg_AddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * MRC Power Tests
 * APIs tested:
 * - le_mrc_SetRadioPower()
 * - le_mrc_GetRadioPower()
 */
//--------------------------------------------------------------------------------------------------
void Testle_mrc_PowerTest
(
    void
)
{
    le_onoff_t    onoff;

    LE_ASSERT(le_mrc_SetRadioPower(LE_OFF) == LE_OK);

    LE_ASSERT(le_mrc_GetRadioPower(&onoff) == LE_OK);
    LE_ASSERT(onoff == LE_OFF);

    LE_ASSERT(le_mrc_SetRadioPower(LE_ON) == LE_OK);

    LE_ASSERT(le_mrc_GetRadioPower(&onoff) == LE_OK);
    LE_ASSERT(onoff == LE_ON);
}

//--------------------------------------------------------------------------------------------------
/*
 * MRC Signal Tests
 * APIs tested:
 * - le_mrc_GetSignalQual()
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_mrc_SignalTest
(
    void
)
{
    uint32_t      quality;

    LE_ASSERT(le_mrc_GetSignalQual(&quality) == LE_OK);
    LE_ASSERT(quality != 0);
}

//--------------------------------------------------------------------------------------------------
/*
 * MRC RAT in use test
 * APIs tested:
 * - le_mrc_GetRadioAccessTechInUse()
 */
//--------------------------------------------------------------------------------------------------
void Testle_mrc_RatInUseTest
(
    void
)
{
    le_mrc_Rat_t  rat;
    size_t i;

    le_mrc_Rat_t  ratList[] = { LE_MRC_RAT_CDMA, LE_MRC_RAT_GSM, LE_MRC_RAT_UMTS,
                                LE_MRC_RAT_LTE, LE_MRC_RAT_CDMA, LE_MRC_RAT_UNKNOWN };

    pa_mrcSimu_SetRadioAccessTechInUse(LE_MRC_RAT_GSM);

    for (i=0; i < 6 ;i++)
    {
        pa_mrcSimu_SetRadioAccessTechInUse(ratList[i]);
        LE_ASSERT(le_mrc_GetRadioAccessTechInUse(&rat) == LE_OK);
        LE_ASSERT(rat == ratList[i]);
    }

    pa_mrcSimu_SetRadioAccessTechInUse(LE_MRC_RAT_GSM);
    LE_ASSERT(le_mrc_GetRadioAccessTechInUse(&rat) == LE_OK);
    LE_ASSERT(rat == LE_MRC_RAT_GSM);
}


//--------------------------------------------------------------------------------------------------
/**
 * MRC Band Preferences mode test
 * APIs tested:
 * - le_mrc_SetBandPreferences()
 * - le_mrc_GetBandPreferences()
 * - le_mrc_SetLteBandPreferences()
 * - le_mrc_GetLteBandPreferences()
 * - le_mrc_SetTdScdmaBandPreferences()
 * - le_mrc_GetTdScdmaBandPreferences()
 */
//--------------------------------------------------------------------------------------------------
void Testle_mrc_BandPreferences
(
    void
)
{
    size_t i;

    le_mrc_BandBitMask_t bandMask = 0;
    le_mrc_BandBitMask_t bandMaskOrigin = 0;
    le_mrc_BandBitMask_t bandList[] = {
                    LE_MRC_BITMASK_BAND_CLASS_0_A_SYSTEM,
                    LE_MRC_BITMASK_BAND_CLASS_0_B_SYSTEM,
                    LE_MRC_BITMASK_BAND_CLASS_1_ALL_BLOCKS,
                    LE_MRC_BITMASK_BAND_CLASS_2_PLACEHOLDER,
                    LE_MRC_BITMASK_BAND_CLASS_3_A_SYSTEM,
                    LE_MRC_BITMASK_BAND_CLASS_4_ALL_BLOCKS,
                    LE_MRC_BITMASK_BAND_CLASS_5_ALL_BLOCKS,
                    LE_MRC_BITMASK_BAND_CLASS_6,
                    LE_MRC_BITMASK_BAND_CLASS_7,
                    LE_MRC_BITMASK_BAND_CLASS_8,
                    LE_MRC_BITMASK_BAND_CLASS_9,
                    LE_MRC_BITMASK_BAND_CLASS_10,
                    LE_MRC_BITMASK_BAND_CLASS_11,
                    LE_MRC_BITMASK_BAND_CLASS_12,
                    LE_MRC_BITMASK_BAND_CLASS_14,
                    LE_MRC_BITMASK_BAND_CLASS_15,
                    LE_MRC_BITMASK_BAND_CLASS_16,
                    LE_MRC_BITMASK_BAND_CLASS_17,
                    LE_MRC_BITMASK_BAND_CLASS_18,
                    LE_MRC_BITMASK_BAND_CLASS_19,
                    LE_MRC_BITMASK_BAND_GSM_DCS_1800,
                    LE_MRC_BITMASK_BAND_EGSM_900,
                    LE_MRC_BITMASK_BAND_PRI_GSM_900,
                    LE_MRC_BITMASK_BAND_GSM_450,
                    LE_MRC_BITMASK_BAND_GSM_480,
                    LE_MRC_BITMASK_BAND_GSM_750,
                    LE_MRC_BITMASK_BAND_GSM_850,
                    LE_MRC_BITMASK_BAND_GSMR_900,
                    LE_MRC_BITMASK_BAND_GSM_PCS_1900,
                    LE_MRC_BITMASK_BAND_WCDMA_EU_J_CH_IMT_2100,
                    LE_MRC_BITMASK_BAND_WCDMA_US_PCS_1900,
                    LE_MRC_BITMASK_BAND_WCDMA_EU_CH_DCS_1800,
                    LE_MRC_BITMASK_BAND_WCDMA_US_1700,
                    LE_MRC_BITMASK_BAND_WCDMA_US_850,
                    LE_MRC_BITMASK_BAND_WCDMA_J_800,
                    LE_MRC_BITMASK_BAND_WCDMA_EU_2600,
                    LE_MRC_BITMASK_BAND_WCDMA_EU_J_900,
                    LE_MRC_BITMASK_BAND_WCDMA_J_1700,
                    0 };

    le_mrc_LteBandBitMask_t lteBandMask;
    le_mrc_LteBandBitMask_t lteBandMaskOrigin;
    le_mrc_LteBandBitMask_t lteBandList[] = {
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_1,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_2,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_3,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_4,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_5,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_6,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_7,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_8,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_9,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_10,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_11,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_12,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_13,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_14,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_17,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_18,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_19,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_20,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_21,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_24,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_25,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_33,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_34,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_35,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_36,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_37,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_38,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_39,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_40,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_41,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_42,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_43,
                    0 };

    le_mrc_TdScdmaBandBitMask_t tdScdmaMask;
    le_mrc_TdScdmaBandBitMask_t tdScdmaMaskOrigin;
    le_mrc_TdScdmaBandBitMask_t bandTdScdmaList[] = {
                    LE_MRC_BITMASK_TDSCDMA_BAND_A,
                    LE_MRC_BITMASK_TDSCDMA_BAND_B,
                    LE_MRC_BITMASK_TDSCDMA_BAND_C,
                    LE_MRC_BITMASK_TDSCDMA_BAND_D,
                    LE_MRC_BITMASK_TDSCDMA_BAND_E,
                    LE_MRC_BITMASK_TDSCDMA_BAND_F,
                    0 };


    // Set/Get 2G/3G Band Preferences.
    LE_ASSERT(le_mrc_GetBandPreferences(&bandMaskOrigin) == LE_OK);
    LE_ASSERT(le_mrc_SetBandPreferences(0) == LE_FAULT);
    for (i=0; bandList[i]; i++)
    {
        LE_ASSERT(le_mrc_SetBandPreferences(bandList[i]) == LE_OK);
        LE_ASSERT(le_mrc_GetBandPreferences(&bandMask) == LE_OK);
        LE_ASSERT(bandMask == bandList[i]);
    }
    LE_ASSERT(le_mrc_SetBandPreferences(bandMaskOrigin) == LE_OK);
    LE_ASSERT(le_mrc_GetBandPreferences(&bandMask) == LE_OK);
    LE_ASSERT(bandMask == bandMaskOrigin);

    // Set/Get LTE Band Preferences.
    LE_ASSERT(le_mrc_GetLteBandPreferences(&lteBandMaskOrigin) == LE_OK);
    LE_ASSERT(le_mrc_SetLteBandPreferences(0) == LE_FAULT);
    for (i=0; lteBandList[i]; i++)
    {
        LE_ASSERT(le_mrc_SetLteBandPreferences(lteBandList[i]) == LE_OK);
        LE_ASSERT(le_mrc_GetLteBandPreferences(&lteBandMask) == LE_OK);
        LE_ASSERT(lteBandMask == lteBandList[i]);
    }
    LE_ASSERT(le_mrc_SetLteBandPreferences(lteBandMaskOrigin) == LE_OK);
    LE_ASSERT(le_mrc_GetLteBandPreferences(&lteBandMask) == LE_OK);
    LE_ASSERT(lteBandMask == lteBandMaskOrigin);

    // Set/Get TdScdma Band Preferences.
    LE_ASSERT(le_mrc_GetTdScdmaBandPreferences(&tdScdmaMaskOrigin) == LE_OK);
    LE_ASSERT(le_mrc_SetTdScdmaBandPreferences(0) == LE_FAULT);
    for (i=0; bandTdScdmaList[i]; i++)
    {
        LE_ASSERT(le_mrc_SetTdScdmaBandPreferences(bandTdScdmaList[i]) == LE_OK);
        LE_ASSERT(le_mrc_GetTdScdmaBandPreferences(&tdScdmaMask) == LE_OK);
        LE_ASSERT(tdScdmaMask == bandTdScdmaList[i]);
    }
    LE_ASSERT(le_mrc_SetTdScdmaBandPreferences(tdScdmaMaskOrigin) == LE_OK);
    LE_ASSERT(le_mrc_GetTdScdmaBandPreferences(&tdScdmaMask) == LE_OK);
    LE_ASSERT(tdScdmaMask == tdScdmaMaskOrigin);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test Register mode.
 * APIs tested:
 *  - le_mrc_SetAutomaticRegisterMode()
 *  - le_mrc_SetManualRegisterMode()
 *  - le_mrc_GetRegisterMode()
 *  - le_mrc_GetCurrentNetworkName()
 *  - le_mrc_GetNetRegState()
 */
//--------------------------------------------------------------------------------------------------
void Testle_mrc_RegisterTest
(
    void
)
{
    char mccHomeStr[LE_MRC_MCC_BYTES] = {0};
    char mccStr[LE_MRC_MNC_BYTES] = {0};
    char mncHomeStr[LE_MRC_MCC_BYTES] = {0};
    char mncStr[LE_MRC_MNC_BYTES] = {0};
    bool isManualOrigin, isManual;
    char nameStr[100] = {0};
    le_mrc_NetRegState_t value;

    // Get the home PLMN to compare results.
    LE_ASSERT(le_sim_GetHomeNetworkMccMnc(LE_SIM_EXTERNAL_SLOT_1, mccHomeStr, LE_MRC_MCC_BYTES,
        mncHomeStr, LE_MRC_MNC_BYTES) == LE_OK);

    LE_INFO("le_sim_GetHomeNetworkMccMnc : mcc.%s mnc.%s", mccHomeStr, mncHomeStr);

    LE_ASSERT(le_mrc_GetRegisterMode(&isManualOrigin,
        mccStr, LE_MRC_MCC_BYTES, mncStr, LE_MRC_MNC_BYTES) == LE_OK);

    LE_ASSERT(le_mrc_SetAutomaticRegisterMode() == LE_OK);

    LE_ASSERT(le_mrc_GetRegisterMode(&isManual, mccStr, LE_MRC_MCC_BYTES,
        mncStr, LE_MRC_MNC_BYTES) == LE_OK);

    LE_ASSERT(le_mrc_GetPlatformSpecificRegistrationErrorCode() == 0);

    LE_ASSERT(isManual == false);

    LE_ASSERT(le_mrc_SetManualRegisterMode("120", mncHomeStr) == LE_OK);
    LE_ASSERT(le_mrc_SetManualRegisterMode("12a", mncHomeStr) == LE_FAULT);
    LE_ASSERT(le_mrc_SetManualRegisterMode("12", mncHomeStr) == LE_FAULT);
    LE_ASSERT(le_mrc_SetManualRegisterMode("12345", mncHomeStr) == LE_FAULT);
    LE_ASSERT(le_mrc_SetManualRegisterMode(mccHomeStr, "a") == LE_FAULT);
    LE_ASSERT(le_mrc_SetManualRegisterMode(mccHomeStr, "abcd") == LE_FAULT);
    LE_ASSERT(le_mrc_SetManualRegisterMode(mccHomeStr, "ggg") == LE_FAULT);

    LE_INFO("le_mrc_SetManualRegisterMode : mcc.%s mnc.%s", mccHomeStr, mncHomeStr);
    LE_ASSERT(le_mrc_SetManualRegisterMode(mccHomeStr, mncHomeStr) == LE_OK);

    LE_ASSERT(le_mrc_GetRegisterMode(&isManual, mccStr, LE_MRC_MCC_BYTES,
        mncStr, LE_MRC_MNC_BYTES) == LE_OK);
    LE_ASSERT(isManual == true);
    LE_ASSERT(strcmp(mccHomeStr, mccStr) == 0);
    LE_ASSERT(strcmp(mncHomeStr, mncStr) == 0);

    LE_ASSERT(le_mrc_SetAutomaticRegisterMode() == LE_OK);

    LE_ASSERT(le_mrc_GetPlatformSpecificRegistrationErrorCode() == 0);

    LE_ASSERT(le_mrc_GetRegisterMode(&isManual, mccStr, LE_MRC_MCC_BYTES,
        mncStr, LE_MRC_MNC_BYTES) == LE_OK);
    LE_ASSERT(isManual == false);

    LE_ASSERT(le_mrc_GetCurrentNetworkName(nameStr, 1) == LE_OVERFLOW);

    LE_ASSERT(le_mrc_GetCurrentNetworkName(nameStr, 100) == LE_OK);

    LE_ASSERT(le_mrc_GetNetRegState(&value) == LE_OK);
    LE_ASSERT(value == LE_MRC_REG_HOME);
}

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

    LE_ASSERT(le_mrc_GetBandCapabilities(&bands) == LE_OK);
    LE_INFO("Get 2G/3G Band Capabilities bit mask: 0x%016"PRIX64, (uint64_t)bands);
    LE_ASSERT(le_mrc_GetLteBandCapabilities(&lteBands) == LE_OK);
    LE_INFO("Get LTE Band Capabilities bit mask: 0x%016"PRIX64, (uint64_t)lteBands);
    LE_ASSERT(le_mrc_GetTdScdmaBandCapabilities(&tdScdmaBands) == LE_OK);
    LE_INFO("Get TD-SCDMA Band Capabilities bit mask: 0x%016"PRIX64, (uint64_t)tdScdmaBands);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Get Tracking area code on LTE network.
 *
 * le_mrc_GetServingCellTracAreaCode() API test
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_GetTac()
{
    uint16_t tac = le_mrc_GetServingCellLteTracAreaCode();

    LE_ASSERT(tac == 0xABCD);
    LE_INFO("le_mrc_GetServingCellLteTracAreaCode returns Tac.0x%X (%d)", tac, tac);
}

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
    le_mrc_NetRegState_t  psState = LE_MRC_REG_UNKNOWN;

    LE_ASSERT_OK(le_mrc_GetPacketSwitchedState(&psState));
    LE_ASSERT(psState == LE_MRC_REG_HOME);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for PS change Notifications.
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
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Set Signal Strength Indication Thresholds
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_SetSignalStrengthIndThresholds
(
    void
)
{

    LE_ASSERT(le_mrc_SetSignalStrengthIndThresholds(LE_MRC_RAT_UNKNOWN, -80,-70)
            == LE_BAD_PARAMETER);
    LE_ASSERT(le_mrc_SetSignalStrengthIndThresholds(LE_MRC_RAT_GSM, -80,-80)
            == LE_BAD_PARAMETER);
    LE_ASSERT(le_mrc_SetSignalStrengthIndThresholds(LE_MRC_RAT_GSM, -70,-80)
            == LE_BAD_PARAMETER);

    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndThresholds(LE_MRC_RAT_GSM, -80,-70));
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndThresholds(LE_MRC_RAT_CDMA, -80,-70));
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndThresholds(LE_MRC_RAT_LTE, -80,-70));
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndThresholds(LE_MRC_RAT_UMTS, -80,-70));
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
    le_mrc_PacketSwitchedChangeHandlerRef_t testHdlrRef;

    testHdlrRef = le_mrc_AddPacketSwitchedChangeHandler(TestPSHandler, NULL);
    LE_ASSERT(testHdlrRef);
    le_mrc_RemovePacketSwitchedChangeHandler(testHdlrRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 * Check The test exit successfully by checking the following trace:
 *  ======== UnitTest of MRC API ends with SUCCESS ========
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // To reactivate for all DEBUG logs
    //le_log_SetFilterLevel(LE_LOG_DEBUG);

    // Init PA simu
    pa_simSimu_Init();
    // Init le_sim
    le_sim_Init();

    // Configure PA SIM simu
    pa_simSimu_SetPIN(PIN_CODE);
    pa_simSimu_SetIMSI(IMSI);
    pa_simSimu_SetCardIdentification(ICCID);
    pa_simSimu_SetHomeNetworkMccMnc(MCC, MNC);
    pa_simSimu_SetHomeNetworkOperator(OPERATOR);
    pa_sim_EnterPIN(PA_SIM_PIN, PIN_CODE);

    // Init and configure PA MRC simu
    mrc_simu_Init();
    // Init le_mrc
    le_mrc_Init();

    LE_INFO("======== Start UnitTest of MRC API ========");

    LE_INFO("======== MRC Power Test ========");
    Testle_mrc_PowerTest();
    LE_INFO("======== MRC Register Test ========");
    Testle_mrc_RegisterTest();
    LE_INFO("======== MRC Signal Test ========");
    Testle_mrc_SignalTest();
    LE_INFO("======== MRC RAT In use Test ========");
    Testle_mrc_RatInUseTest();
    LE_INFO("======== MRC Band Preferences Test ========");
    Testle_mrc_BandPreferences();
    LE_INFO("======== MRC Get Band Capabilities Test ========");
    Testle_mrc_GetBandCapabilities();
    LE_INFO("======== MRC Get TAC Test ========");
    Testle_mrc_GetTac();

    LE_INFO("======== GetPSState Test ========");
    Testle_mrc_GetPSState();
    LE_INFO("======== GetPSState Test PASSED ========");

    LE_INFO("======== PSHdlr Test ========");
    Testle_mrc_PSHdlr();
    LE_INFO("======== PSHdlr Test PASSED ========");

    LE_INFO("======== le_mrc_SetSignalStrengthIndThresholds Test ========");
    Testle_mrc_SetSignalStrengthIndThresholds();
    LE_INFO("======== le_mrc_SetSignalStrengthIndThresholds Test PASSED ========");

    LE_INFO("======== UnitTest of MRC API ends with SUCCESS ========");

    exit(EXIT_SUCCESS);
}
