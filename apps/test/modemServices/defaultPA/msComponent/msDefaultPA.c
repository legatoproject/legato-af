/**
 * Simple test of modemServices based on the default PA for hl76.
 *
 * The purpose of this test is to make sure the port of modemServices on default PA will not cause
 * any crash or other issues. The functionality of modemServices is not tested. So, most test cases
 * are expected to return LE_FAULT or LE_UNSUPPORTED.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Run mdc test on default PA
 */
//--------------------------------------------------------------------------------------------------
static void mdc_DefaultPATest()
{

    le_mdc_ProfileRef_t profileRef;
    LE_TEST_OUTPUT(" == Start mdc test on default PA == ");

    LE_TEST_ASSERT(le_mdc_NumProfiles() > 0, "Test le_mdc_NumProfiles()");

    profileRef = le_mdc_GetProfile((uint32_t)LE_MDC_DEFAULT_PROFILE);
    LE_TEST_OK(NULL == profileRef, "Test le_mdc_GetProfile() with default profile");

    profileRef = le_mdc_GetProfile(1);
    LE_TEST_OK(NULL != profileRef, "Test le_mdc_GetProfile() with index 1");
    LE_TEST_OK(LE_FAULT == le_mdc_MapProfileOnNetworkInterface(profileRef, "rmnet_data0"),
               "Test le_mdc_MapProfileOnNetworkInterface()");

    le_mdc_ConState_t state = LE_MDC_DISCONNECTED;
    LE_TEST_OK(LE_FAULT == le_mdc_GetSessionState(profileRef, &state),
               "Test le_mdc_GetSessionState()");

    LE_TEST_OK(LE_FAULT == le_mdc_SetPDP(profileRef, LE_MDC_PDP_IPV4), "Test le_mdc_SetPDP()");
#if LE_CONFIG_ENABLE_DEFAULT_APN_SWITCHING
    LE_TEST_OK(LE_FAULT == le_mdc_SetDefaultAPN(profileRef), "Test le_mdc_SetDefaultAPN()");
#else
    LE_TEST_OK(LE_UNSUPPORTED == le_mdc_SetDefaultAPN(profileRef), "Test le_mdc_SetDefaultAPN()");
#endif
    LE_TEST_OK(LE_FAULT == le_mdc_SetAPN(profileRef, "sp.telus.com"), "Test le_mdc_SetAPN()");
    LE_TEST_OK(LE_FAULT == le_mdc_SetAuthentication(profileRef,
                                                    LE_MDC_AUTH_PAP,
                                                    "userName",
                                                    "password"),
               "Test le_mdc_SetAuthentication()");

    LE_TEST_ASSERT(LE_FAULT == le_mdc_StartSession(profileRef), "Test le_mdc_StartSession()");
    LE_TEST_OK(LE_FAULT == le_mdc_ResetBytesCounter(), "Test le_mdc_ResetBytesCounter()");

    LE_TEST_OK(!le_mdc_IsIPv4(profileRef), "Test le_mdc_IsIPv4()");
    char ipAddr[10] = {0};
    char dns1Addr[10] = {0};
    char dns2Addr[10] = {0};
    char gwayAddr[10] = {0};
    LE_TEST_OK(LE_FAULT == le_mdc_GetIPv4Address(profileRef, ipAddr, sizeof(ipAddr)),
               "Test le_mdc_GetIPv4Address()");
    LE_TEST_OK(LE_FAULT == le_mdc_GetIPv4GatewayAddress(profileRef, gwayAddr, sizeof(gwayAddr)),
               "Test le_mdc_GetIPv4GatewayAddress()");
    LE_TEST_OK(LE_FAULT == le_mdc_GetIPv4DNSAddresses(profileRef,
                                                      dns1Addr, sizeof(dns1Addr),
                                                      dns2Addr, sizeof(dns2Addr)),
               "Test le_mdc_GetIPv4DNSAddresses()");

    LE_TEST_ASSERT(LE_FAULT == le_mdc_StopSession(profileRef), "Test le_mdc_StopSession()");

    LE_TEST_OUTPUT(" == End mdc test on default PA == ");
    le_thread_Sleep(3);
}

//--------------------------------------------------------------------------------------------------
/**
 * Run riPin test on default PA
 */
//--------------------------------------------------------------------------------------------------
static void riPin_DefaultPATest()
{
    bool amIOwner;

    LE_TEST_OUTPUT(" == Start riPin test on default PA == ");

    LE_TEST_OK(LE_FAULT == le_riPin_AmIOwnerOfRingSignal(&amIOwner),
               "Test le_riPin_AmIOwnerOfRingSignal()");

    LE_TEST_OK(LE_FAULT == le_riPin_TakeRingSignal(), "Test le_riPin_TakeRingSignal()");

    LE_TEST_OK(LE_FAULT == le_riPin_ReleaseRingSignal(), "Test le_riPin_ReleaseRingSignal()");

    LE_TEST_OUTPUT(" == End riPin test on default PA == ");
}

//--------------------------------------------------------------------------------------------------
/**
 * Run mrc test on default PA
 */
//--------------------------------------------------------------------------------------------------
static void mrc_DefaultPATest()
{
    uint8_t state;
    le_mrc_RatBitMask_t bitMaskOrigin = 0;
    le_mrc_Rat_t rat;
    le_mrc_NetRegState_t netRegState;
    uint32_t quality;

    LE_TEST_OUTPUT(" == Start mrc test on default PA == ");

    LE_TEST_OK(LE_UNSUPPORTED == le_mrc_SetSarBackoffState(0),
               "Test le_mrc_SetSarBackoffState()");

    LE_TEST_OK(LE_UNSUPPORTED == le_mrc_GetSarBackoffState(&state),
               "Test le_mrc_GetSarBackoffState()");

    LE_TEST_OK(NULL == le_mrc_PerformPciNetworkScan(LE_MRC_BITMASK_RAT_LTE),
               "Test le_mrc_PerformPciNetworkScan()");

    LE_TEST_OK(NULL == le_mrc_GetFirstPciScanInfo(NULL), "Test le_mrc_GetFirstPciScanInfo()");

    LE_TEST_OK(LE_FAULT == le_mrc_GetRatPreferences(&bitMaskOrigin),
               "Test le_mrc_GetRatPreferences()");

    LE_TEST_OK(LE_FAULT == le_mrc_GetRadioAccessTechInUse(&rat),
               "Test le_mrc_GetRadioAccessTechInUse()");

    LE_TEST_OK(LE_FAULT == le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_ALL),
               "Test le_mrc_SetRatPreferences() with LE_MRC_BITMASK_RAT_ALL");

    LE_TEST_OK(LE_UNSUPPORTED == le_mrc_SetRatPreferences(LE_MRC_BITMASK_RAT_LTE),
               "Test le_mrc_SetRatPreferences() with LE_MRC_BITMASK_RAT_LTE");

    LE_TEST_OK(LE_FAULT == le_mrc_GetRadioAccessTechInUse(&rat),
               "Test le_mrc_GetRadioAccessTechInUse()");

    LE_TEST_OK(NULL == le_mrc_PerformCellularNetworkScan(bitMaskOrigin),
               "Test le_mrc_PerformCellularNetworkScan()");

    LE_TEST_OK(LE_FAULT == le_mrc_SetRadioPower(LE_ON), "Test le_mrc_SetRadioPower()");

    LE_TEST_OK(LE_FAULT == le_mrc_GetNetRegState(&netRegState), "Test le_mrc_GetNetRegState()");

    LE_TEST_OK(LE_FAULT == le_mrc_GetSignalQual(&quality), "Test le_mrc_GetSignalQual()");

    LE_TEST_OK(LE_FAULT == le_mrc_GetPacketSwitchedState(&netRegState),
               "Test le_mrc_GetPacketSwitchedState()");

    LE_TEST_OK(UINT32_MAX == le_mrc_GetServingCellId(), "Test le_mrc_GetServingCellId()");

    LE_TEST_OK(UINT32_MAX == le_mrc_GetServingCellLocAreaCode(),
               "Test le_mrc_GetServingCellLocAreaCode()");

    LE_TEST_OK(UINT16_MAX == le_mrc_GetServingCellLteTracAreaCode(),
               "Test le_mrc_GetServingCellLteTracAreaCode()");

    LE_TEST_OK(NULL == le_mrc_MeasureSignalMetrics(), "Test le_mrc_MeasureSignalMetrics()");

    LE_TEST_OK(NULL == le_mrc_GetNeighborCellsInfo(), "Test le_mrc_GetNeighborCellsInfo()");

    char mcc[LE_MRC_MCC_BYTES] = {0};
    char mnc[LE_MRC_MNC_BYTES] = {0};
    LE_TEST_OK(LE_FAULT == le_mrc_GetCurrentNetworkMccMnc(mcc,
                                                          LE_MRC_MCC_BYTES-1,
                                                          mnc,
                                                          LE_MRC_MNC_BYTES),
               "Test le_mrc_GetCurrentNetworkMccMnc()");

    char nameStr[10];
    LE_TEST_OK(LE_FAULT == le_mrc_GetCurrentNetworkName(nameStr, sizeof(nameStr)),
               "Test le_mrc_GetCurrentNetworkName()");

    le_mrc_BandBitMask_t bandBitMask;
    LE_TEST_OK(LE_FAULT == le_mrc_GetBandPreferences(&bandBitMask),
               "Test le_mrc_GetBandPreferences()");

    LE_TEST_OK(LE_FAULT == le_mrc_SetBandPreferences(bandBitMask),
               "Test le_mrc_SetBandPreferences()");

    LE_TEST_OK(LE_FAULT == le_mrc_GetLteBandPreferences(&bandBitMask),
               "Test le_mrc_GetLteBandPreferences()");

    LE_TEST_OK(LE_FAULT == le_mrc_SetSignalStrengthIndDelta(LE_MRC_RAT_GSM,1),
               "Test le_mrc_SetSignalStrengthIndDelta()");

    char mccStr[LE_MRC_MNC_BYTES] = {0};
    char mncStr[LE_MRC_MNC_BYTES] = {0};
    bool isManualOrigin;
    LE_TEST_OK(LE_FAULT == le_mrc_GetRegisterMode(&isManualOrigin,
                                                  mccStr,
                                                  LE_MRC_MCC_BYTES,
                                                  mncStr,
                                                  LE_MRC_MNC_BYTES),
               "Test le_mrc_GetRegisterMode()");

    LE_TEST_OK(LE_FAULT == le_mrc_SetAutomaticRegisterMode(),
               "Test le_mrc_SetAutomaticRegisterMode()");

    le_mrc_BandBitMask_t bands;
    LE_TEST_OK(LE_UNSUPPORTED == le_mrc_GetBandCapabilities(&bands),
               "Test le_mrc_GetBandCapabilities()");

    le_mrc_LteBandBitMask_t lteBands;
    LE_TEST_OK(LE_UNSUPPORTED == le_mrc_GetLteBandCapabilities(&lteBands),
               "Test le_mrc_GetLteBandCapabilities()");

    LE_TEST_OUTPUT(" == End mrc test on default PA == ");
}

//--------------------------------------------------------------------------------------------------
/**
 * Run sim test on default PA
 */
//--------------------------------------------------------------------------------------------------
static void sim_DefaultPATest()
{
    LE_TEST_OUTPUT(" == Start sim test on default PA == ");

    LE_TEST_OK(LE_SIM_STATE_UNKNOWN == le_sim_GetState(LE_SIM_UNSPECIFIED),
               "Test le_sim_GetState()");

    LE_TEST_OK(LE_FAULT == le_sim_SelectCard(LE_SIM_EXTERNAL_SLOT_1),
               "Test le_sim_SelectCard()");

    char iccid[22]; // Expect larger than 21
    LE_TEST_OK(LE_FAULT == le_sim_GetICCID(LE_SIM_EXTERNAL_SLOT_1, iccid, sizeof(iccid)),
               "Test le_sim_GetICCID()");

    char eid[34]; // Expect larger than 33
    LE_TEST_OK(LE_FAULT == le_sim_GetEID(LE_SIM_EXTERNAL_SLOT_1, eid, sizeof(eid)),
               "Test le_sim_GetEID()");

    char imsi[20];
    LE_TEST_OK(LE_FAULT == le_sim_GetIMSI(LE_SIM_EXTERNAL_SLOT_1, imsi, sizeof(imsi)),
               "Test le_sim_GetIMSI()");

    char phoneNumber[20];
    LE_TEST_OK(LE_FAULT == le_sim_GetSubscriberPhoneNumber(LE_SIM_EXTERNAL_SLOT_1,
                                                           phoneNumber,
                                                           sizeof(phoneNumber)),
               "Test le_sim_GetSubscriberPhoneNumber()");

    LE_TEST_OK(!le_sim_IsPresent(LE_SIM_EXTERNAL_SLOT_1), "Test le_sim_IsPresent()");

    LE_TEST_OK(!le_sim_IsReady(LE_SIM_EXTERNAL_SLOT_1), "Test le_sim_IsReady()");

    const char* pin = "12345";
    LE_TEST_OK(LE_NOT_FOUND == le_sim_EnterPIN(LE_SIM_EXTERNAL_SLOT_1, pin),
               "Test le_sim_EnterPIN()");

    LE_TEST_OK(LE_NOT_FOUND == le_sim_ChangePIN(LE_SIM_EXTERNAL_SLOT_1, pin, pin),
               "Test le_sim_ChangePIN()");

    LE_TEST_OK(LE_NOT_FOUND == le_sim_GetRemainingPINTries(LE_SIM_EXTERNAL_SLOT_1),
               "Test le_sim_GetRemainingPINTries()");

    uint32_t puk;
    LE_TEST_OK(LE_NOT_FOUND == le_sim_GetRemainingPUKTries(LE_SIM_EXTERNAL_SLOT_1, &puk),
               "Test le_sim_GetRemainingPUKTries()");

    LE_TEST_OK(LE_NOT_FOUND == le_sim_Unlock(LE_SIM_EXTERNAL_SLOT_1, pin), "Test le_sim_Unlock()");

    LE_TEST_OK(LE_NOT_FOUND == le_sim_Lock(LE_SIM_EXTERNAL_SLOT_1, pin), "Test le_sim_Lock()");

    const char* pukStr = "12345678";
    LE_TEST_OK(LE_NOT_FOUND == le_sim_Unblock(LE_SIM_EXTERNAL_SLOT_1, pukStr, pin),
               "Test le_sim_Unblock()");

    LE_TEST_OK(LE_SIM_STATE_UNKNOWN == le_sim_GetState(LE_SIM_EXTERNAL_SLOT_1),
               "Test le_sim_GetState()");

    char netName[20];
    LE_TEST_OK(LE_FAULT == le_sim_GetHomeNetworkOperator(LE_SIM_EXTERNAL_SLOT_1,
                                                         netName,
                                                         sizeof(netName)),
               "Test le_sim_GetHomeNetworkOperator()");

    char mcc[4];
    char mnc[4];
    LE_TEST_OK(LE_FAULT == le_sim_GetHomeNetworkMccMnc(LE_SIM_EXTERNAL_SLOT_1,
                                                       mcc,
                                                       sizeof(mcc),
                                                       mnc,
                                                       sizeof(mnc)),
               "Test le_sim_GetHomeNetworkMccMnc()");
    LE_TEST_OK(LE_FAULT == le_sim_AcceptSimToolkitCommand(LE_SIM_EXTERNAL_SLOT_1),
               "Test le_sim_AcceptSimToolkitCommand()");

    LE_TEST_OK(LE_FAULT == le_sim_RejectSimToolkitCommand(LE_SIM_EXTERNAL_SLOT_1),
               "Test le_sim_RejectSimToolkitCommand()");

    le_sim_StkRefreshMode_t mode;
    LE_TEST_OK(LE_FAULT == le_sim_GetSimToolkitRefreshMode(LE_SIM_EXTERNAL_SLOT_1, &mode),
               "Test le_sim_GetSimToolkitRefreshMode()");

    le_sim_StkRefreshStage_t stage;
    LE_TEST_OK(LE_FAULT == le_sim_GetSimToolkitRefreshStage(LE_SIM_EXTERNAL_SLOT_1, &stage),
               "Test le_sim_GetSimToolkitRefreshStage()");

    size_t rspImsiLen2 = 100;
    uint8_t rspImsi2[100];
    uint8_t swi1, swi2;
    char dfGsmPath[] = "3F007FFF";
    LE_TEST_OK(LE_UNSUPPORTED == le_sim_SendCommand(LE_SIM_EXTERNAL_SLOT_1,
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
                                                    &rspImsiLen2),
               "Test le_sim_SendCommand()");

    LE_TEST_OK(LE_UNSUPPORTED == le_sim_SetAutomaticSelection(true),
               "Test le_sim_SetAutomaticSelection()");

    bool enable;
    LE_TEST_OK(LE_UNSUPPORTED == le_sim_GetAutomaticSelection(&enable),
               "Test le_sim_GetAutomaticSelection()");

    LE_TEST_OK(LE_FAULT == le_sim_Reset(LE_SIM_EXTERNAL_SLOT_1), "Test le_sim_Reset()");

    LE_TEST_OK(LE_FAULT == le_sim_SetPower(LE_SIM_EXTERNAL_SLOT_1, LE_OFF),
               "Test le_sim_SetPower()");

    LE_TEST_OUTPUT(" == End sim test on default PA == ");
}

//--------------------------------------------------------------------------------------------------
/**
 * Run lpt test on default PA
 */
//--------------------------------------------------------------------------------------------------
static void lpt_DefaultPATest()
{
    LE_TEST_OUTPUT(" == Start lpt test on default PA == ");

    LE_TEST_OK(LE_UNSUPPORTED == le_lpt_SetEDrxState(LE_LPT_EDRX_RAT_LTE_M1, LE_OFF),
               "Test le_lpt_SetEDrxState()");

    LE_TEST_OK(LE_UNSUPPORTED == le_lpt_SetRequestedEDrxValue(LE_LPT_EDRX_RAT_LTE_M1, 1),
               "Test le_lpt_SetRequestedEDrxValue()");

    uint8_t value;
    LE_TEST_OK(LE_UNSUPPORTED == le_lpt_GetRequestedEDrxValue(LE_LPT_EDRX_RAT_LTE_M1, &value),
               "Test le_lpt_GetRequestedEDrxValue()");

    LE_TEST_OK(LE_UNSUPPORTED == le_lpt_GetNetworkProvidedEDrxValue(LE_LPT_EDRX_RAT_LTE_M1,
                                                                    &value),
               "Test le_lpt_GetNetworkProvidedEDrxValue()");

    LE_TEST_OK(LE_UNSUPPORTED == le_lpt_GetNetworkProvidedPagingTimeWindow(LE_LPT_EDRX_RAT_LTE_M1,
                                                                           &value),
               "Test le_lpt_GetNetworkProvidedPagingTimeWindow()");

    LE_TEST_OUTPUT(" == End lpt test on default PA == ");
}

//--------------------------------------------------------------------------------------------------
/**
 * Run temp test on default PA
 */
//--------------------------------------------------------------------------------------------------
static void temp_DefaultPATest()
{
    LE_TEST_OUTPUT(" == Start temp test on default PA == ");

    const char* sensorName = "POWER_AMPLIFIER";
    le_temp_SensorRef_t tmSensorRef;
    LE_TEST_OK(NULL == (tmSensorRef= le_temp_Request(sensorName)), "Test le_temp_Request()");

    char name[20];
    LE_TEST_OK(LE_FAULT == le_temp_GetSensorName(tmSensorRef, name, sizeof(name)),
               "Test le_temp_GetSensorName()");

    LE_TEST_OK(LE_FAULT == le_temp_StartMonitoring(), "Test le_temp_StartMonitoring()");

    LE_TEST_OUTPUT(" == End temp test on default PA == ");
}

//--------------------------------------------------------------------------------------------------
/**
 * Run sms test on default PA
 */
//--------------------------------------------------------------------------------------------------
static void sms_DefaultPATest()
{
    LE_TEST_OUTPUT(" == Start sms test on default PA == ");

    LE_TEST_OK(LE_FAULT == le_sms_ClearCellBroadcastIds(), "Test le_sms_ClearCellBroadcastIds()");

    char sms[50];
    LE_TEST_OK(LE_FAULT == le_sms_GetSmsCenterAddress(sms, sizeof(sms)),
               "Test le_sms_GetSmsCenterAddress()");

    LE_TEST_OK(LE_FAULT == le_sms_ClearCdmaCellBroadcastServices(),
               "Test le_sms_ClearCdmaCellBroadcastServices()");

    LE_TEST_OK(LE_FAULT == le_sms_ClearCellBroadcastIds(), "Test le_sms_ClearCellBroadcastIds()");

    LE_TEST_OK(LE_FAULT == le_sms_RemoveCellBroadcastIds(1,1),
               "Test le_sms_RemoveCellBroadcastIds()");

    LE_TEST_OK(LE_FAULT == le_sms_AddCellBroadcastIds(1,1),
               "Test le_sms_AddCellBroadcastIds()");

    LE_TEST_OK(LE_FAULT == le_sms_DeactivateCdmaCellBroadcast(),
               "Test le_sms_DeactivateCdmaCellBroadcast()");

    char smsCenter[] = "sms center";
    LE_TEST_OK(LE_FAULT == le_sms_GetSmsCenterAddress(smsCenter, sizeof(smsCenter)),
               "Test le_sms_GetSmsCenterAddress()");

    LE_TEST_OUTPUT(" == End sms test on default PA == ");
}

//--------------------------------------------------------------------------------------------------
/**
 * Run ips test on default PA
 */
//--------------------------------------------------------------------------------------------------
static void ips_DefaultPATest()
{
    LE_TEST_OUTPUT(" == Start ips test on default PA == ");

    uint32_t volt;
    LE_TEST_OK(LE_FAULT == le_ips_GetInputVoltage(&volt), "Test le_ips_GetInputVoltage()");

    LE_TEST_OUTPUT(" == End ips test on default PA == ");
}

//--------------------------------------------------------------------------------------------------
/**
 * Component Init
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);
    LE_TEST_OUTPUT(" ======= Start modemServices test on default PA ======= ");

    // Due to limited thread keys, before fixing the insufficient key issue, only six service
    // clients can be tested here, and they can be any six service clients in this file.
    mdc_DefaultPATest();
    riPin_DefaultPATest();
    mrc_DefaultPATest();
    lpt_DefaultPATest();
    temp_DefaultPATest();
    sim_DefaultPATest();
    sms_DefaultPATest();
    ips_DefaultPATest();

    LE_TEST_OUTPUT(" ======= End modemServices test on default PA ======= ");
    LE_TEST_EXIT;

    pthread_exit(0);
}
