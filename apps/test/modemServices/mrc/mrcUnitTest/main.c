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
 * Semaphore for thread synchronization (jamming detection test and PCI scan sync test)
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t     ThreadSemaphore;

//--------------------------------------------------------------------------------------------------
/**
 * PCI scan async thread reference.
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t  PciThreadRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Timeout for semaphore
 */
//--------------------------------------------------------------------------------------------------
static le_clk_Time_t    TimeToWait = { 0, 1000000 };

//--------------------------------------------------------------------------------------------------
/**
 * Client Session Reference for the current message received from a client
 */
//--------------------------------------------------------------------------------------------------
#define APPLICATION_NB        2

//--------------------------------------------------------------------------------------------------
/**
 * Maximum and default values for SAR backoff state
 */
//--------------------------------------------------------------------------------------------------
#define SAR_BACKOFF_STATE_MAX       8
#define SAR_BACKOFF_STATE_DEFAULT   0

//--------------------------------------------------------------------------------------------------
/**
 * Thread context
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t                                    appId;          ///< Application Id
    le_thread_Ref_t                             appThreadRef;   ///< Thread reference
    le_mrc_JammingDetectionEventHandlerRef_t    stateHandler;   ///< Jamming handler
} AppContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * Context for simulated applications
 */
//--------------------------------------------------------------------------------------------------
static AppContext_t AppCtx[APPLICATION_NB];

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
 * Get the server service reference (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_sim_GetServiceRef
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
le_msg_SessionRef_t le_sim_GetClientSessionRef
(
    void
)
{
    return _ClientSessionRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set a client session reference value
 */
//--------------------------------------------------------------------------------------------------
static void SetClientSessionRef
(
    uint32_t*   valuePtr,      ///< [IN] New value
    bool        isNull         ///< [IN] Set to NULL is true
)
{
    if (isNull)
    {
        _ClientSessionRef = NULL;
    }
    else
    {
        _ClientSessionRef = (le_msg_SessionRef_t)valuePtr;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Synchronize test thread (i.e. main) and tasks
 *
 */
//--------------------------------------------------------------------------------------------------
static void SynchTest
(
    int number
)
{
    int i = 0;

    for (i = 0; i < number; i++)
    {
        LE_ASSERT(le_sem_WaitWithTimeOut(ThreadSemaphore, TimeToWait) == LE_OK);
    }
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
                                LE_MRC_RAT_LTE, LE_MRC_RAT_CDMA,
                                LE_MRC_RAT_TDSCDMA, LE_MRC_RAT_UNKNOWN };

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
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_26,
                    LE_MRC_BITMASK_LTE_BAND_E_UTRA_OP_BAND_28,
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
    le_mrc_NeighborCellsRef_t ngbrRef;

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

    ngbrRef = le_mrc_GetNeighborCellsInfo();
    LE_ASSERT(!ngbrRef);
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
    LE_ASSERT_OK(le_mrc_SetSignalStrengthIndThresholds(LE_MRC_RAT_TDSCDMA, -80,-70));
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Set Signal Strength Indication delta
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_SetSignalStrengthIndDelta
(
    void
)
{
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
    LE_INFO("Jamming report");
    switch(report)
    {
        case LE_MRC_JAMMING_REPORT_FINAL:
            LE_DEBUG("FINAL REPORT");
            break;

        case LE_MRC_JAMMING_REPORT_INTERMEDIATE:
            LE_DEBUG("INTERMEDIATE REPORT");
            break;

        default:
            LE_DEBUG("Unsupported report");
            return;
    }

    switch(status)
    {
        case LE_MRC_JAMMING_STATUS_UNKNOWN:
            LE_DEBUG("Unknown\n");
            break;

        case LE_MRC_JAMMING_STATUS_NULL:
            LE_DEBUG("NULL\n");
            break;

        case LE_MRC_JAMMING_STATUS_LOW:
            LE_DEBUG("Low\n");
            break;

        case LE_MRC_JAMMING_STATUS_MEDIUM:
            LE_DEBUG("Medium");
            break;

        case LE_MRC_JAMMING_STATUS_HIGH:
            LE_DEBUG("High");
            break;

        case LE_MRC_JAMMING_STATUS_JAMMED:
            LE_DEBUG("Jammed!!!!");
            break;

        default:
            LE_DEBUG("Invalid status");
            break;
    }
    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test tasks: this function handles the task and run an eventLoop
 *
 */
//--------------------------------------------------------------------------------------------------
static void* AppHandler
(
    void* ctxPtr
)
{
    AppContext_t* appCtxPtr = (AppContext_t*)ctxPtr;
    LE_ASSERT(NULL != appCtxPtr);

    // Subscribe to SIM state handler
    appCtxPtr->stateHandler = le_mrc_AddJammingDetectionEventHandler(TestJammingHandler, NULL);
    LE_ASSERT(NULL != appCtxPtr->stateHandler);

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(ThreadSemaphore);

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove jamming detection handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveJammingHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    AppContext_t* appCtxPtr = (AppContext_t*)param1Ptr;
    LE_ASSERT(appCtxPtr);

    le_mrc_RemoveJammingDetectionEventHandler(appCtxPtr->stateHandler);

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * MRC Jamming detection Tests
 * APIs tested:
 * - le_mrc_StartJammingDetection()
 * - le_mrc_StopJammingDetection()
 * - le_mrc_AddJammingDetectionEventHandler()
 * - le_mrc_RemoveJammingDetectionEventHandler()
 */
//--------------------------------------------------------------------------------------------------
void Testle_mrc_JammingTest
(
    void
)
{
    #define THREAD_NAME_LEN 15
    int i = 0;

    uint32_t clientSessionRef1 = 0x1234;
    uint32_t clientSessionRef2 = 0x4321;

    char string[THREAD_NAME_LEN]={0};

    // Test NULL cases
    LE_ASSERT(!le_mrc_AddJammingDetectionEventHandler(NULL, NULL));

    // Create a semaphore to coordinate the test
    ThreadSemaphore = le_sem_Create("HandlerSem", 0);

    // int app context
    memset(AppCtx, 0, APPLICATION_NB*sizeof(AppContext_t));

    // Simulate one application which subcribes to jamming handler using
    // le_mrc_AddJammingDetectionEventHandler
    snprintf(string, THREAD_NAME_LEN, "app%dhandler", i);
    AppCtx[i].appId = i;
    AppCtx[i].appThreadRef = le_thread_Create(string, AppHandler, &AppCtx[i]);
    le_thread_Start(AppCtx[i].appThreadRef);

    // Wait that the tasks have started before continuing the test
    SynchTest(1);

    // Set client session reference for application 1
    SetClientSessionRef(&clientSessionRef1, false);

    // Set jamming detection feature to unsupported
    pa_mrcSimu_SetJammingDetection(PA_MRCSIMU_JAMMING_UNSUPPORTED);
    LE_ASSERT(LE_UNSUPPORTED == le_mrc_StartJammingDetection());

    // Stop jamming but the application does not started it: LE_FAULT is expected
    LE_ASSERT(LE_FAULT == le_mrc_StopJammingDetection());

    // Set jamming detection feature to deactivated
    pa_mrcSimu_SetJammingDetection(PA_MRCSIMU_JAMMING_DEACTIVATED);
    LE_ASSERT_OK(le_mrc_StartJammingDetection());
    LE_ASSERT(LE_DUPLICATE == le_mrc_StartJammingDetection());
    LE_ASSERT(PA_MRCSIMU_JAMMING_ACTIVATED == pa_mrcSimu_GetJammingDetection());

    pa_mrcSimu_ReportJammingDetection(LE_MRC_JAMMING_REPORT_INTERMEDIATE,
                                      LE_MRC_JAMMING_STATUS_LOW);
    SynchTest(1);

    LE_ASSERT_OK(le_mrc_StopJammingDetection());
    LE_ASSERT(PA_MRCSIMU_JAMMING_DEACTIVATED == pa_mrcSimu_GetJammingDetection());


    // Simulate two application with one handler for each application
    i++;
    snprintf(string, THREAD_NAME_LEN, "app%dhandler", i);
    AppCtx[i].appId = i;
    AppCtx[i].appThreadRef = le_thread_Create(string, AppHandler, &AppCtx[i]);
    le_thread_Start(AppCtx[i].appThreadRef);

    // Wait that the tasks have started before continuing the test
    SynchTest(1);

    /*le_event_QueueFunctionToThread(AppCtx[1].appThreadRef, AddJammingHandler, &AppCtx[1], NULL);
    SynchTest(1);*/
    pa_mrcSimu_SetJammingDetection(PA_MRCSIMU_JAMMING_DEACTIVATED);

    // Start jamming with application 1
    SetClientSessionRef(&clientSessionRef1, false);
    LE_ASSERT_OK(le_mrc_StartJammingDetection());
    LE_ASSERT(PA_MRCSIMU_JAMMING_ACTIVATED == pa_mrcSimu_GetJammingDetection());

    // Start jamming with application 2
    SetClientSessionRef(&clientSessionRef2, false);
    LE_ASSERT_OK(le_mrc_StartJammingDetection());
    LE_ASSERT(PA_MRCSIMU_JAMMING_ACTIVATED == pa_mrcSimu_GetJammingDetection());

    pa_mrcSimu_ReportJammingDetection(LE_MRC_JAMMING_REPORT_INTERMEDIATE,
                                      LE_MRC_JAMMING_STATUS_LOW);

    SynchTest(APPLICATION_NB);

    // Stop jamming with application 2
    LE_ASSERT_OK(le_mrc_StopJammingDetection());
    LE_ASSERT(PA_MRCSIMU_JAMMING_ACTIVATED == pa_mrcSimu_GetJammingDetection());

    // Stop jamming with application 1
    SetClientSessionRef(&clientSessionRef1, false);
    LE_ASSERT_OK(le_mrc_StopJammingDetection());
    LE_ASSERT(PA_MRCSIMU_JAMMING_DEACTIVATED == pa_mrcSimu_GetJammingDetection());

    for (i = 0; i<APPLICATION_NB; i++)
    {
        le_event_QueueFunctionToThread(AppCtx[i].appThreadRef,
                                       RemoveJammingHandler,
                                       &AppCtx[i],
                                       NULL);
       le_thread_Cancel(AppCtx[i].appThreadRef);
    }

    SetClientSessionRef(0, true);
}

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

    LE_ASSERT_OK(le_mrc_GetSarBackoffState(&state));
    LE_ASSERT(SAR_BACKOFF_STATE_DEFAULT == state);

    for (i = SAR_BACKOFF_STATE_DEFAULT; i <= SAR_BACKOFF_STATE_MAX; i++)
    {
        LE_ASSERT_OK(le_mrc_SetSarBackoffState(i));
        LE_ASSERT_OK(le_mrc_GetSarBackoffState(&state));
        LE_INFO("Backoff state: %d", state);
        LE_ASSERT(i == state);
    }

    LE_ASSERT(LE_OUT_OF_RANGE == le_mrc_SetSarBackoffState(SAR_BACKOFF_STATE_MAX+1));
}

//--------------------------------------------------------------------------------------------------
/**
 * Function used to get the default Mcc and Mnc (Mcc = 001, Mnc = 01)
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_MccMnc
(
    void
)
{
    char mccHomeStr[LE_MRC_MCC_BYTES] = {0};
    char mncHomeStr[LE_MRC_MNC_BYTES] = {0};

    LE_INFO("Get the default mcc and mnc of home network");
    LE_ASSERT_OK(le_sim_GetHomeNetworkMccMnc(LE_SIM_EXTERNAL_SLOT_1, mccHomeStr, LE_MRC_MCC_BYTES,
                                             mncHomeStr, LE_MRC_MNC_BYTES));
    LE_ASSERT(0 == strcmp(mccHomeStr,PA_SIMU_SIM_DEFAULT_MCC));
    LE_ASSERT(0 == strcmp(mncHomeStr,PA_SIMU_SIM_DEFAULT_MNC));
    LE_INFO("Home network mcc.%s mnc.%s", mccHomeStr, mncHomeStr);

    LE_INFO("Set the  mcc and mnc 208 and 01");
    pa_simSimu_SetHomeNetworkMccMnc(MCC, MNC);
    LE_ASSERT_OK(le_sim_GetHomeNetworkMccMnc(LE_SIM_EXTERNAL_SLOT_1, mccHomeStr, LE_MRC_MCC_BYTES,
                                             mncHomeStr, LE_MRC_MNC_BYTES));
    LE_ASSERT(0 == strcmp(mccHomeStr,MCC));
    LE_ASSERT(0 == strcmp(mncHomeStr,MNC));
    LE_INFO("Home network mcc.%s mnc.%s", mccHomeStr, mncHomeStr);
}

//--------------------------------------------------------------------------------------------------
/**
 * MRC PCI scan feature
 * APIs tested:
 * - le_mrc_PerformPciNetworkScan()
 * - le_mrc_PerformPciNetworkScanAsync()
 * - le_mrc_GetFirstPciScanInfo()
 * - le_mrc_GetNextPciScanInfo()
 * - le_mrc_GetFirstPlmnInfo()
 * - le_mrc_GetNextPlmnInfo()
 * - le_mrc_GetPciScanCellId()
 * - le_mrc_GetPciScanMccMnc()
 * - le_mrc_DeletePciNetworkScan()
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_PciScan
(
    void
)
{
    uint16_t plmnNbr = 0, expectedCellId = 0;
    uint16_t cellId = 0;
    char mcc[LE_MRC_MCC_BYTES] = {0};
    char mnc[LE_MRC_MNC_BYTES] = {0};
    le_mrc_PciScanInformationListRef_t scanInfoListRef = NULL;
    le_mrc_PciScanInformationRef_t     scanInfoRef = NULL;
    le_mrc_PlmnInformationRef_t        plmnInfoRef = NULL;

    LE_ASSERT(NULL == le_mrc_PerformPciNetworkScan(LE_MRC_BITMASK_RAT_GSM));
    LE_ASSERT(NULL == le_mrc_PerformPciNetworkScan(LE_MRC_BITMASK_RAT_UMTS));
    scanInfoListRef = le_mrc_PerformPciNetworkScan(LE_MRC_BITMASK_RAT_LTE);
    LE_ASSERT(scanInfoListRef != NULL);

    LE_ASSERT(NULL == le_mrc_GetFirstPciScanInfo(NULL));
    scanInfoRef = le_mrc_GetFirstPciScanInfo(scanInfoListRef);
    LE_ASSERT(NULL != scanInfoRef);

    do
    {
        LE_ASSERT(LE_FAULT == (int16_t)le_mrc_GetPciScanCellId(NULL));
        cellId = le_mrc_GetPciScanCellId(scanInfoRef);

        LE_ASSERT(NULL == le_mrc_GetFirstPlmnInfo(NULL));
        plmnInfoRef = le_mrc_GetFirstPlmnInfo(scanInfoRef);
        LE_ASSERT(NULL != plmnInfoRef);

        plmnNbr = 0;

        do
        {


            LE_ASSERT(LE_FAULT == le_mrc_GetPciScanMccMnc(NULL,
                                                          mcc,
                                                          LE_MRC_MCC_BYTES,
                                                          mnc,
                                                          LE_MRC_MCC_BYTES));

            LE_ASSERT(LE_FAULT == le_mrc_GetPciScanMccMnc(plmnInfoRef,
                                                          mcc,
                                                          LE_MRC_MCC_BYTES,
                                                          NULL,
                                                          LE_MRC_MCC_BYTES));

            LE_ASSERT(LE_OVERFLOW == le_mrc_GetPciScanMccMnc(plmnInfoRef,
                                                             mcc,
                                                             0,
                                                             mnc,
                                                             LE_MRC_MCC_BYTES));

            LE_ASSERT(LE_OVERFLOW == le_mrc_GetPciScanMccMnc(plmnInfoRef,
                                                             mcc,
                                                             LE_MRC_MCC_BYTES,
                                                             mnc,
                                                             0));

            LE_ASSERT(LE_OK == le_mrc_GetPciScanMccMnc(plmnInfoRef,
                                                       mcc,
                                                       LE_MRC_MCC_BYTES,
                                                       mnc,
                                                       LE_MRC_MCC_BYTES));

            // Check returned MCC and MNC values for each cell
            char expectedMcc[LE_MRC_MCC_BYTES] = {0};
            char expectedMnc[LE_MRC_MNC_BYTES] = {0};
            snprintf(expectedMnc, sizeof(expectedMnc), "%d", plmnNbr);
            snprintf(expectedMcc, sizeof(expectedMcc), "2%d", plmnNbr);
            LE_ASSERT(0 == strncmp(expectedMnc, mnc, sizeof(mnc)));
            LE_ASSERT(0 == strncmp(expectedMcc, mcc, sizeof(mcc)));

            plmnNbr++;
            LE_ASSERT(NULL == le_mrc_GetNextPlmnInfo(NULL));
            plmnInfoRef = le_mrc_GetNextPlmnInfo(scanInfoRef);
        }
        while (plmnInfoRef);

        LE_ASSERT(NULL == le_mrc_GetNextPciScanInfo(NULL));
        scanInfoRef = le_mrc_GetNextPciScanInfo(scanInfoListRef);

        // Check returned CellID value
        LE_ASSERT(expectedCellId == cellId);
        LE_ASSERT(plmnNbr == cellId+1);
        expectedCellId++;
    }
    while(scanInfoRef);

    scanInfoRef = le_mrc_GetFirstPciScanInfo(scanInfoListRef);
    LE_ASSERT(NULL != scanInfoRef);
    le_mrc_DeletePciNetworkScan(scanInfoListRef);
    scanInfoRef = le_mrc_GetFirstPciScanInfo(scanInfoListRef);
    LE_ASSERT(NULL == scanInfoRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler for PCI scan result
 */
//--------------------------------------------------------------------------------------------------
static void PciScanResultHandler
(
    le_mrc_PciScanInformationListRef_t listRef,
    void* contextPtr
)
{
    LE_ASSERT(NULL != listRef);
    LE_ASSERT(NULL != le_mrc_GetFirstPciScanInfo(listRef));
    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread for asynchronous PCI scan test.
 */
//--------------------------------------------------------------------------------------------------
static void* PciScanThread
(
    void* context
)
{
    le_mrc_PerformPciNetworkScanAsync(LE_MRC_BITMASK_RAT_LTE, PciScanResultHandler, NULL);
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * MRC PCI scan async feature
 * APIs tested:
 * - le_mrc_PerformPciNetworkScanAsync()
 * - le_mrc_GetFirstPciScanInfo()
 */
//--------------------------------------------------------------------------------------------------
static void Testle_mrc_PciScanAsync
(
    void
)
{
    le_clk_Time_t time = { .sec = 120000 };

    ThreadSemaphore = le_sem_Create("ThreadSemaphore",0);
    PciThreadRef = le_thread_Create("PciThread", PciScanThread, NULL);
    le_thread_Start(PciThreadRef);

    // Wait for PCI scan completion
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(ThreadSemaphore, time));

    le_thread_Cancel(PciThreadRef);
    le_sem_Delete(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread used to run SIM unit tests
 *
 */
//--------------------------------------------------------------------------------------------------
static void* TestThread
(
    void* context
)
{
    LE_INFO("======== Start UnitTest of MRC API ========");

    LE_INFO("======== MRC MccMnc Test ========");
    Testle_mrc_MccMnc();
    LE_INFO("======== MRC Power Test ========");
    Testle_mrc_SarBackoff();
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
    LE_INFO("======== MRC PSState Test ========");
    Testle_mrc_GetPSState();
    LE_INFO("======== MRC PSHdlr Test ========");
    Testle_mrc_PSHdlr();
    LE_INFO("======== MRC Signal strength thresholds Test ========");
    Testle_mrc_SetSignalStrengthIndThresholds();
    LE_INFO("======== MRC Signal strength delta Test ========");
    Testle_mrc_SetSignalStrengthIndDelta();
    LE_INFO("======== MRC Jamming detection Test ========");
    Testle_mrc_JammingTest();
    LE_INFO("======== MRC PCI scan Test ========");
    Testle_mrc_PciScan();
    LE_INFO("======== MRC PCI scan async Test ========");
    Testle_mrc_PciScanAsync();

    LE_INFO("======== UnitTest of MRC API ends with SUCCESS ========");

    exit(0);

    return NULL;
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
    pa_simSimu_SetHomeNetworkOperator(OPERATOR);
    pa_sim_EnterPIN(PA_SIM_PIN, PIN_CODE);

    // Init and configure PA MRC simu
    mrc_simu_Init();

    // Init le_mrc
    le_mrc_Init();

    le_thread_Start(le_thread_Create("TestThread", TestThread, NULL));
}
