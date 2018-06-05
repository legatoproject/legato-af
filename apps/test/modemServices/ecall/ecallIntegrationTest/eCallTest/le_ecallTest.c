
 /**
  * This module implements the le_ecall's unit tests.
  *
 * You must issue the following commands:
 * @verbatim
   $ app runProc eCallTest --exe=eCallTest -- <PSAP number>
   @endverbatim

  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"
#include "mdmCfgEntries.h"

//--------------------------------------------------------------------------------------------------
/**
 * Minimum value of ERA GLONASS Call Cleardown Fallback Timer (CCFT) expressed in minutes
 */
//--------------------------------------------------------------------------------------------------
#define ERA_GLONASS_CCFT_MIN              1

//--------------------------------------------------------------------------------------------------
/**
 * Maximum value of ERA GLONASS Call Cleardown Fallback Timer (CCFT) expressed in minutes
 */
//--------------------------------------------------------------------------------------------------
#define ERA_GLONASS_CCFT_MAX             720


//--------------------------------------------------------------------------------------------------
/**
 * MSD configurations for testing purposes
 */
//--------------------------------------------------------------------------------------------------
#if 0
// keep different testing Msd configurations.
// VIN: WM9VDSVDSYA123456
static uint8_t ImportedMsd[] =
                      {0x01, 0x5C, 0x06, 0x81, 0xD5, 0x49, 0x70, 0xD6, 0x5C, 0x35, 0x97, 0xCA,
                      0x04, 0x20, 0xC4, 0x14, 0x67, 0xF1, 0x03, 0xAD, 0xE6, 0x8A, 0xC5, 0x2E,
                      0x9B, 0xB8, 0x41, 0x3F, 0x14, 0x9C, 0x07, 0x41, 0x4F, 0xB4, 0x14, 0xF6,
                      0x01, 0x01, 0x80, 0x81, 0x3E, 0x82, 0x18, 0x18, 0x23, 0x23, 0x00};

// MSD with maximum length
static uint8_t ImportedMsd[] =
                      {0x01, 0x01, 0x7e, 0x02, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
                      0x30, 0x30, 0x7e, 0x03, 0x00, 0x00, 0x7e, 0x04, 0x00, 0x01, 0x7e, 0x05,
                      0x02, 0x7e, 0x06, 0x3c, 0x7e, 0x07, 0x88, 0x42, 0x00, 0x32, 0x7e, 0x08,
                      0x01, 0x7e, 0x09, 0x00, 0x00, 0x52, 0x7e, 0x10, 0x01, 0x00, 0x7d, 0x02,
                      0x00, 0x7d, 0x03, 0x00, 0x7d, 0x04, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x7d, 0x05, 0x00, 0x7d, 0x06, 0x00, 0x00, 0x00, 0x00, 0x7d, 0x07, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7d, 0x08, 0x00, 0x7d, 0x09,
                      0x00, 0x00, 0x00, 0x7d, 0x0a, 0x00, 0x00, 0x00, 0x7d, 0x0b, 0x00, 0x7e,
                      0x20, 0x01, 0x00, 0x7d, 0x02, 0x00, 0x7d, 0x03, 0x00, 0x7d, 0x04, 0x00,
                      0x7d, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif

// VIN: ASDAJNPR1VABCDEFG
static uint8_t ImportedMsd[] =
                      {0x01, 0x4C, 0x07, 0x80, 0xA6, 0x4D, 0x29, 0x25, 0x97, 0x60, 0x17, 0x0A,
                       0x2C, 0xC3, 0x4E, 0x3D, 0x05, 0x1B, 0x18, 0x48, 0x61, 0xEB, 0xA0, 0xC8,
                       0xFF, 0x73, 0x7E, 0x64, 0x20, 0xD1, 0x04, 0x01, 0x3F, 0x81, 0x00};

//--------------------------------------------------------------------------------------------------
/**
 * PSAP number
 */
//--------------------------------------------------------------------------------------------------
static const char*         PsapNumber = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Reference to the last eCall test
 */
//--------------------------------------------------------------------------------------------------
static le_ecall_CallRef_t   LastTestECallRef;

//--------------------------------------------------------------------------------------------------
/**
 * Flag indicating whether a MSD has already been sent during a current session.
 */
//--------------------------------------------------------------------------------------------------
static bool IsMsdSentOnce = false;

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for eCall state Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyECallEventHandler
(
    le_ecall_CallRef_t  eCallRef,
    le_ecall_State_t    state,
    void*               contextPtr
)
{

    LE_INFO("eCall TEST: New eCall state: %d for eCall ref.%p", state, eCallRef);

    LE_INFO("eCall state from get function %d", le_ecall_GetState(eCallRef));

    switch(state)
    {
        case LE_ECALL_STATE_STARTED:
        {
            IsMsdSentOnce = false;
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_STARTED.");
            break;
        }
        case LE_ECALL_STATE_CONNECTED:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_CONNECTED.");
            break;
        }
        case LE_ECALL_STATE_DISCONNECTED:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_DISCONNECTED.");
            LE_INFO("Termination reason: %d", le_ecall_GetTerminationReason(eCallRef) );
            break;
        }
        case LE_ECALL_STATE_WAITING_PSAP_START_IND:
        {
            LE_INFO("Check MyECallEventHandler passed, state is \
                                  LE_ECALL_STATE_WAITING_PSAP_START_IND.");
            break;
        }
        case LE_ECALL_STATE_PSAP_START_IND_RECEIVED:
        {
            LE_INFO("Check MyECallEventHandler passed, state is \
                                  LE_ECALL_STATE_PSAP_START_IND_RECEIVED.");
            if (IsMsdSentOnce)
            {
                // The MSD has been already sent once, I update the MSD when I receive again the
                // LE_ECALL_STATE_PSAP_START_IND_RECEIVED event.
                LE_INFO("UpdateMSD");
                LE_ASSERT(le_ecall_ImportMsd(eCallRef, ImportedMsd, sizeof(ImportedMsd)) == LE_OK);
                if (le_ecall_SendMsd(eCallRef) != LE_OK)
                {
                    LE_ERROR("Could not send the MSD");
                }
            }
            else
            {
                LE_INFO("1st MSD sending...");
                if (le_ecall_SendMsd(eCallRef) == LE_OK)
                {
                    IsMsdSentOnce = true;
                }
                else
                {
                    LE_ERROR("Could not send the MSD");
                }
            }
            break;
        }
        case LE_ECALL_STATE_MSD_TX_STARTED:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_MSD_TX_STARTED.");
            break;
        }
        case LE_ECALL_STATE_LLNACK_RECEIVED:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_LLNACK_RECEIVED.");
            break;
        }
        case LE_ECALL_STATE_LLACK_RECEIVED:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_LLACK_RECEIVED.");
            break;
        }
        case LE_ECALL_STATE_MSD_TX_COMPLETED:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_MSD_TX_COMPLETED.");
            break;
        }
        case LE_ECALL_STATE_MSD_TX_FAILED:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_MSD_TX_FAILED.");
            break;
        }
        case LE_ECALL_STATE_ALACK_RECEIVED_POSITIVE:
        {
            LE_INFO("Check MyECallEventHandler passed, state is \
                                LE_ECALL_STATE_ALACK_RECEIVED_POSITIVE.");
            break;
        }
        case LE_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN:
        {
            LE_INFO("Check MyECallEventHandler passed, state is \
                                LE_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN.");
            break;
        }
        case LE_ECALL_STATE_STOPPED:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_STOPPED.");
            break;
        }
        case LE_ECALL_STATE_RESET:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_RESET.");
            break;
        }
        case LE_ECALL_STATE_COMPLETED:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_COMPLETED.");
            break;
        }
        case LE_ECALL_STATE_FAILED:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_FAILED.");
            break;
        }
        case LE_ECALL_STATE_END_OF_REDIAL_PERIOD:
        {
            LE_INFO("Check MyECallEventHandler passed, state is \
                                         LE_ECALL_STATE_END_OF_REDIAL_PERIOD.");
            break;
        }
        case LE_ECALL_STATE_TIMEOUT_T2:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_TIMEOUT_T2.");
            break;
        }
        case LE_ECALL_STATE_TIMEOUT_T3:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_TIMEOUT_T3.");
            break;
        }
        case LE_ECALL_STATE_TIMEOUT_T5:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_TIMEOUT_T5.");
            break;
        }
        case LE_ECALL_STATE_TIMEOUT_T6:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_TIMEOUT_T6.");
            break;
        }
        case LE_ECALL_STATE_TIMEOUT_T7:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_TIMEOUT_T7.");
            break;
        }
        case LE_ECALL_STATE_TIMEOUT_T9:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_TIMEOUT_T9.");
            break;
        }
        case LE_ECALL_STATE_TIMEOUT_T10:
        {
            LE_INFO("Check MyECallEventHandler passed, state is LE_ECALL_STATE_TIMEOUT_T10.");
            break;
        }
        case LE_ECALL_STATE_UNKNOWN:
        default:
        {
            LE_INFO("Check MyECallEventHandler failed, unknown state.");
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
//                                       Test Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Test: Set/Get Operation mode.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_ecall_OperationMode
(
    void
)
{
    le_result_t         res;
    le_ecall_OpMode_t   mode = LE_ECALL_NORMAL_MODE;

    //! [OpModes]
    LE_ASSERT((res=le_ecall_ForceOnlyMode()) == LE_OK);
    LE_ASSERT((res=le_ecall_GetConfiguredOperationMode(&mode)) == LE_OK);
    LE_ASSERT(mode == LE_ECALL_ONLY_MODE);

    LE_ASSERT((res=le_ecall_ForcePersistentOnlyMode()) == LE_OK);
    LE_ASSERT((res=le_ecall_GetConfiguredOperationMode(&mode)) == LE_OK);
    LE_ASSERT(mode == LE_ECALL_FORCED_PERSISTENT_ONLY_MODE);

    LE_ASSERT((res=le_ecall_ExitOnlyMode()) == LE_OK);
    LE_ASSERT((res=le_ecall_GetConfiguredOperationMode(&mode)) == LE_OK);
    LE_ASSERT(mode == LE_ECALL_NORMAL_MODE);
    //! [OpModes]
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Configuration settings.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_ecall_ConfigSettings
(
    void
)
{
    le_ecall_MsdTxMode_t      mode           = LE_ECALL_TX_MODE_PULL;

    LE_INFO("Start Testle_ecall_ConfigSettings");

    LE_ASSERT_OK(le_ecall_UseUSimNumbers());

    //! [PsapNumber]
    LE_ASSERT_OK(le_ecall_SetPsapNumber("0102030405"));

    char psapNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0};
    LE_ASSERT_OK(le_ecall_GetPsapNumber(psapNumber, sizeof(psapNumber)));
    LE_INFO("PSAP number: %s", psapNumber);
    //! [PsapNumber]
    LE_ASSERT(0 == strncmp(psapNumber, "0102030405", strlen("0102030405")));
    LE_ASSERT(LE_OVERFLOW == le_ecall_GetPsapNumber(psapNumber, 1));

    //! [TxMode]
    LE_ASSERT(le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PUSH) == LE_OK);
    LE_ASSERT(le_ecall_GetMsdTxMode(&mode) == LE_OK);
    //! [TxMode]
    LE_ASSERT(mode == LE_ECALL_TX_MODE_PUSH);

    //! [NadTime]
    LE_ASSERT_OK(le_ecall_SetNadDeregistrationTime(180));

    uint16_t deregistrationTime = 0;
    LE_ASSERT_OK(le_ecall_GetNadDeregistrationTime(&deregistrationTime));
    LE_INFO("Deregistration time: %d minutes", deregistrationTime);
    //! [NadTime]
    LE_ASSERT(deregistrationTime == 180);

    //! [Standard]
    LE_ASSERT_OK(le_ecall_SetSystemStandard(LE_ECALL_ERA_GLONASS));

    le_ecall_SystemStandard_t systemStandard;
    LE_ASSERT_OK(le_ecall_GetSystemStandard(&systemStandard));
    LE_INFO("System standard: %d", systemStandard);
    //! [Standard]
    LE_ASSERT(LE_ECALL_ERA_GLONASS == systemStandard);

    //! [MsdVersion]
    LE_ASSERT_OK(le_ecall_SetMsdVersion(1))

    uint32_t msdVersion = 0;
    LE_ASSERT_OK(le_ecall_GetMsdVersion(&msdVersion));
    LE_INFO("MSD version: %d", msdVersion);
    //! [MsdVersion]
    LE_ASSERT(1 == msdVersion);

    //! [Vehicle]
    LE_ASSERT_OK(le_ecall_SetVehicleType(LE_ECALL_MSD_VEHICLE_BUS_M2));

    le_ecall_MsdVehicleType_t vehicleType;
    LE_ASSERT_OK(le_ecall_GetVehicleType(&vehicleType));
    LE_INFO("Vehicle type: %d", vehicleType);
    //! [Vehicle]
    LE_ASSERT(LE_ECALL_MSD_VEHICLE_BUS_M2 == vehicleType);

    LE_ASSERT(LE_FAULT == le_ecall_SetVIN("VF37"));
    LE_ASSERT(LE_FAULT == le_ecall_SetVIN("VF37BRiVE12345678"));
    LE_ASSERT(LE_FAULT == le_ecall_SetVIN("VF37IRFVE12345678"));
    LE_ASSERT(LE_FAULT == le_ecall_SetVIN("VF37BoFVE12345678"));
    LE_ASSERT(LE_FAULT == le_ecall_SetVIN("VFO7BRFVE12345678"));
    LE_ASSERT(LE_FAULT == le_ecall_SetVIN("VF37BRFVE12345q78"));
    LE_ASSERT(LE_FAULT == le_ecall_SetVIN("VF37BRFVE12Q45678"));
    LE_ASSERT(LE_FAULT == le_ecall_SetVIN("iIoOqQFVE12345678"));
    LE_ASSERT(LE_FAULT == le_ecall_SetVIN("VF37BRFVE02345678"));
    LE_ASSERT(LE_FAULT == le_ecall_SetVIN("VF37BRFVEu2345678"));
    LE_ASSERT(LE_FAULT == le_ecall_SetVIN("VF37BRFVEU2345678"));
    LE_ASSERT(LE_FAULT == le_ecall_SetVIN("VF37BRFVEz2345678"));
    LE_ASSERT(LE_FAULT == le_ecall_SetVIN("VF37BRFVEZ2345678"));

    //! [VIN]
    LE_ASSERT_OK(le_ecall_SetVIN("VF37BRFVE12345678"));

    char vin[LE_ECALL_VIN_MAX_BYTES] = {0};
    LE_ASSERT_OK(le_ecall_GetVIN(vin, sizeof(vin)));
    LE_INFO("VIN: %s", vin);
    //! [VIN]
    LE_ASSERT(0 == strcmp("VF37BRFVE12345678", vin));
    LE_ASSERT(LE_BAD_PARAMETER == le_ecall_GetVIN(vin, LE_ECALL_VIN_MAX_LEN));

    //! [Propulsion]
    LE_ASSERT_OK(le_ecall_SetPropulsionType(LE_ECALL_PROPULSION_TYPE_OTHER));

    le_ecall_PropulsionTypeBitMask_t propulsionType;
    LE_ASSERT_OK(le_ecall_GetPropulsionType(&propulsionType));
    LE_INFO("Propulsion type: 0x%02x", propulsionType);
    //! [Propulsion]
    LE_ASSERT(LE_ECALL_PROPULSION_TYPE_OTHER == propulsionType);

    propulsionType = LE_ECALL_PROPULSION_TYPE_ELECTRIC;
    LE_ASSERT_OK(le_ecall_SetPropulsionType(propulsionType));

    propulsionType = LE_ECALL_PROPULSION_TYPE_GASOLINE;
    LE_ASSERT_OK(le_ecall_GetPropulsionType(&propulsionType));
    LE_ASSERT( LE_ECALL_PROPULSION_TYPE_ELECTRIC == propulsionType );

    propulsionType = LE_ECALL_PROPULSION_TYPE_HYDROGEN;
    LE_ASSERT_OK(le_ecall_SetPropulsionType(propulsionType));
    LE_ASSERT_OK(le_ecall_GetPropulsionType(&propulsionType));
    LE_ASSERT(LE_ECALL_PROPULSION_TYPE_HYDROGEN == propulsionType);

    //! [DialTimer]
    LE_ASSERT_OK(le_ecall_SetIntervalBetweenDialAttempts(30));

    uint16_t pause = 0;
    LE_ASSERT_OK(le_ecall_GetIntervalBetweenDialAttempts(&pause));
    LE_INFO("Interval between dial attempts: %d seconds", pause);
    //! [DialTimer]
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: ERA-GLONASS settings.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_ecall_EraGlonassSettings
(
    void
)
{
    uint16_t             attempts = 0;
    uint16_t             duration = 0;
    le_ecall_CallRef_t   testECallRef = 0x00;
    uint16_t             deregistrationTime = 0;
    le_result_t          res;

    LE_INFO("Start Testle_ecall_EraGlonassSettings");

    LE_ASSERT(NULL != (testECallRef=le_ecall_Create()));

    //! [FallbackTimer]
    /* Check that default value is within the allowed range */
    LE_ASSERT_OK(le_ecall_GetEraGlonassFallbackTime(&duration));
    LE_ASSERT(duration <= ERA_GLONASS_CCFT_MAX);
    LE_ASSERT(duration >= ERA_GLONASS_CCFT_MIN);

    /* Check that values above the maximum value are rejected */
    duration = 0;
    LE_ASSERT(LE_FAULT == le_ecall_SetEraGlonassFallbackTime(ERA_GLONASS_CCFT_MAX+1));
    LE_ASSERT_OK(le_ecall_GetEraGlonassFallbackTime(&duration));
    LE_ASSERT(duration <= ERA_GLONASS_CCFT_MAX);

    /* Check that values below the minimum value are rejected */
    duration = 0;
    LE_ASSERT(LE_FAULT == le_ecall_SetEraGlonassFallbackTime(ERA_GLONASS_CCFT_MIN-1));
    LE_ASSERT_OK(le_ecall_GetEraGlonassFallbackTime(&duration));
    LE_ASSERT(duration >= ERA_GLONASS_CCFT_MIN);

    /* Check that the minimum value can be set */
    duration = 0;
    LE_ASSERT_OK(le_ecall_SetEraGlonassFallbackTime(ERA_GLONASS_CCFT_MIN));
    LE_ASSERT_OK(le_ecall_GetEraGlonassFallbackTime(&duration));
    LE_ASSERT(duration == ERA_GLONASS_CCFT_MIN);

    /* Check that the maximum value can be set */
    duration = 0;
    LE_ASSERT_OK(le_ecall_SetEraGlonassFallbackTime(ERA_GLONASS_CCFT_MAX));
    LE_ASSERT_OK(le_ecall_GetEraGlonassFallbackTime(&duration));
    LE_ASSERT(duration == ERA_GLONASS_CCFT_MAX);

    /* Check that a value within the range can be set */
    duration = 0;
    LE_ASSERT_OK(le_ecall_SetEraGlonassFallbackTime(30));
    LE_ASSERT_OK(le_ecall_GetEraGlonassFallbackTime(&duration));
    LE_ASSERT(duration == 30);

    //! [NadTime]
    LE_ASSERT_OK(le_ecall_SetNadDeregistrationTime(200));

    LE_ASSERT_OK(le_ecall_GetNadDeregistrationTime(&deregistrationTime));
    LE_INFO("Deregistration time: %d minutes", deregistrationTime);
    //! [NadTime]
    LE_ASSERT(deregistrationTime == 200);

    res = le_ecall_SetEraGlonassPostTestRegistrationTime(0);
    LE_ASSERT((LE_OK == res) || ((LE_UNSUPPORTED == res)));

    res = le_ecall_GetEraGlonassPostTestRegistrationTime(&duration);
    LE_ASSERT((LE_OK == res) || ((LE_UNSUPPORTED == res)));
    if (LE_OK == res)
    {
        LE_ASSERT(0 == duration);
    }

    //! [PostTest]
    res = le_ecall_SetEraGlonassPostTestRegistrationTime(500);
    LE_ASSERT((LE_OK == res) || ((LE_UNSUPPORTED == res)));

    res = le_ecall_GetEraGlonassPostTestRegistrationTime(&duration);
    LE_ASSERT((LE_OK == res) || ((LE_UNSUPPORTED == res)));
    LE_INFO("Post Test registration time: %d seconds", duration);
    //! [PostTest]
    if (LE_OK == res)
    {
        LE_ASSERT(500 == duration);
    }

    //! [AutoAnswerTimer]
    /* Check that a valid value can be set */
    duration = 0;
    LE_ASSERT_OK(le_ecall_SetEraGlonassAutoAnswerTime(30));
    LE_ASSERT_OK(le_ecall_GetEraGlonassAutoAnswerTime(&duration));
    LE_ASSERT(30 == duration);

    //! [MSDMaxTransTime]
    duration = 0;
    LE_ASSERT_OK(le_ecall_SetEraGlonassMSDMaxTransmissionTime(60));
    LE_ASSERT_OK(le_ecall_GetEraGlonassMSDMaxTransmissionTime(&duration));
    LE_ASSERT(60 == duration);

    //! [DialConfig]
    LE_ASSERT_OK(le_ecall_SetEraGlonassManualDialAttempts(7));
    LE_ASSERT_OK(le_ecall_GetEraGlonassManualDialAttempts(&attempts));
    LE_ASSERT(attempts == 7);

    LE_ASSERT_OK(le_ecall_SetEraGlonassAutoDialAttempts(9));
    LE_ASSERT_OK(le_ecall_GetEraGlonassAutoDialAttempts(&attempts));
    LE_ASSERT(attempts == 9);

    LE_ASSERT_OK(le_ecall_SetEraGlonassDialDuration(240));
    LE_ASSERT_OK(le_ecall_GetEraGlonassDialDuration(&duration));
    LE_ASSERT(duration == 240);
    //! [DialConfig]

    //! [OptData]
    /* Crash Severity configuration */
    LE_ASSERT_OK(le_ecall_SetMsdEraGlonassCrashSeverity(testECallRef, 0));
    LE_ASSERT_OK(le_ecall_ResetMsdEraGlonassCrashSeverity(testECallRef));
    LE_ASSERT_OK(le_ecall_SetMsdEraGlonassCrashSeverity(testECallRef, 99));

    /* DataDiagnosticResult configuration */
    LE_ASSERT_OK(le_ecall_SetMsdEraGlonassDiagnosticResult(testECallRef, 0x3FFFFFFFFFF));
    LE_ASSERT_OK(le_ecall_SetMsdEraGlonassDiagnosticResult(testECallRef, 0));
    LE_ASSERT_OK(le_ecall_ResetMsdEraGlonassDiagnosticResult(testECallRef));
    LE_ASSERT_OK(le_ecall_SetMsdEraGlonassDiagnosticResult(testECallRef,
                 LE_ECALL_DIAG_RESULT_PRESENT_MIC_CONNECTION_FAILURE));

    /* CrashInfo configuration */
    LE_ASSERT_OK(le_ecall_SetMsdEraGlonassCrashInfo(testECallRef, 0xFFFF));
    LE_ASSERT_OK(le_ecall_SetMsdEraGlonassCrashInfo(testECallRef, 0));
    LE_ASSERT_OK(le_ecall_ResetMsdEraGlonassCrashInfo(testECallRef));
    LE_ASSERT_OK(le_ecall_SetMsdEraGlonassCrashInfo(testECallRef,
                 LE_ECALL_CRASH_INFO_PRESENT_CRASH_FRONT_OR_SIDE |
                 LE_ECALL_CRASH_INFO_CRASH_FRONT_OR_SIDE));

    /* Coordinate system type configuration */
    uint32_t msdVersion = 0;
    LE_ASSERT_OK(le_ecall_GetMsdVersion(&msdVersion));

    /* If MSD version is 2, set following MSD parameters */
    if (2 == msdVersion)
    {
        LE_ASSERT_OK(le_ecall_SetMsdEraGlonassCoordinateSystemType(testECallRef,
                                                 LE_ECALL_MSD_COORDINATE_SYSTEM_TYPE_PZ90));
        LE_ASSERT_OK(le_ecall_ResetMsdEraGlonassCoordinateSystemType(testECallRef));
        LE_ASSERT_OK(le_ecall_SetMsdEraGlonassCoordinateSystemType(testECallRef,
                                                 LE_ECALL_MSD_COORDINATE_SYSTEM_TYPE_WGS84));
    }
    //! [OptData]

    le_ecall_Delete(testECallRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Import or set MSD elements.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_ecall_LoadMsd
(
    void
)
{
    le_ecall_CallRef_t   testECallRef = 0x00;
    uint8_t              exportMsd[LE_ECALL_MSD_MAX_LEN] = {0};
    size_t               msdSize = 0;
    LE_INFO("Start Testle_ecall_LoadMsd");

    LE_ASSERT((testECallRef=le_ecall_Create()) != NULL);

    //! [SetMsd]
    LE_ASSERT(le_ecall_SetMsdPosition(testECallRef, true, +48898064, +2218092, 0) == LE_OK);
    LE_ASSERT(le_ecall_SetMsdPositionN1(testECallRef,511,511) == LE_OK);
    LE_ASSERT(le_ecall_SetMsdPositionN2(testECallRef,-512,-512) == LE_OK);

    LE_ASSERT(le_ecall_SetMsdPassengersCount(testECallRef, 3) == LE_OK);

    //! [ExportMsd]
    LE_ASSERT_OK(le_ecall_ExportMsd(testECallRef, exportMsd, &msdSize));

    // Check LE_DUPLICATE on le_ecall_SetMsdPosition and le_ecall_SetMsdPassengersCount
    LE_ASSERT(le_ecall_ImportMsd(testECallRef, ImportedMsd, sizeof(ImportedMsd)) == LE_OK);

    LE_ASSERT(le_ecall_ExportMsd(testECallRef, exportMsd, &msdSize) == LE_DUPLICATE);
    //! [ExportMsd]

    LE_ASSERT(le_ecall_SetMsdPosition(testECallRef, true, +48070380, -11310000, 45) ==
                                                                                LE_DUPLICATE);
    LE_ASSERT(le_ecall_SetMsdPositionN1(testECallRef, 511, 511) == LE_DUPLICATE);
    LE_ASSERT(le_ecall_SetMsdPositionN2(testECallRef, -512, -512) == LE_DUPLICATE);
    LE_ASSERT(le_ecall_SetMsdPassengersCount(testECallRef, 3) == LE_DUPLICATE);
    //! [SetMsd]
    LE_ASSERT(le_ecall_ResetMsdEraGlonassCrashSeverity(testECallRef) == LE_DUPLICATE);
    LE_ASSERT(le_ecall_SetMsdEraGlonassCrashSeverity(testECallRef, 0) == LE_DUPLICATE);
    LE_ASSERT(le_ecall_ResetMsdEraGlonassDiagnosticResult(testECallRef) == LE_DUPLICATE);
    LE_ASSERT(le_ecall_SetMsdEraGlonassDiagnosticResult(testECallRef,
              LE_ECALL_DIAG_RESULT_PRESENT_MIC_CONNECTION_FAILURE)
              == LE_DUPLICATE);
    LE_ASSERT(le_ecall_ResetMsdEraGlonassCrashInfo(testECallRef) == LE_DUPLICATE);
    LE_ASSERT(le_ecall_SetMsdEraGlonassCrashInfo(testECallRef,
              LE_ECALL_CRASH_INFO_PRESENT_CRASH_FRONT_OR_SIDE
              | LE_ECALL_CRASH_INFO_CRASH_FRONT_OR_SIDE)
              == LE_DUPLICATE);
    LE_ASSERT(le_ecall_ResetMsdEraGlonassCoordinateSystemType(testECallRef) == LE_DUPLICATE);
    LE_ASSERT(le_ecall_SetMsdEraGlonassCoordinateSystemType(testECallRef,
                                LE_ECALL_MSD_COORDINATE_SYSTEM_TYPE_WGS84) == LE_DUPLICATE);

    le_ecall_Delete(testECallRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Create and start a manual eCall.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_ecall_StartManual
(
    void
)
{
    le_ecall_CallRef_t  testECallRef = 0x00;
    le_ecall_State_t    state;
    char                psap[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];

    LE_INFO("Start Testle_ecall_StartManual");

    LE_ASSERT(le_ecall_SetPsapNumber(PsapNumber) == LE_OK);
    LE_ASSERT(le_ecall_GetPsapNumber(psap, sizeof(psap)) == LE_OK);
    LE_INFO("psap %s", psap);
    LE_ASSERT(strncmp(psap, PsapNumber, strlen(PsapNumber)) == 0);

    LE_ASSERT(le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PUSH) == LE_OK);

    //! [EcallSession]
    LE_ASSERT((testECallRef=le_ecall_Create()) != NULL);

    LE_ASSERT(le_ecall_ImportMsd(testECallRef, ImportedMsd, sizeof(ImportedMsd)) == LE_OK);

    LE_ASSERT(le_ecall_StartManual(testECallRef) == LE_OK);

    LE_ASSERT(le_ecall_StartTest(testECallRef) == LE_BUSY);
    LE_ASSERT(le_ecall_StartAutomatic(testECallRef) == LE_BUSY);

    LE_ASSERT(le_ecall_End(testECallRef) == LE_OK);

    state=le_ecall_GetState(testECallRef);
    LE_ASSERT(((state>=LE_ECALL_STATE_STARTED) && (state<=LE_ECALL_STATE_FAILED)));

    le_ecall_Delete(testECallRef);
    //! [EcallSession]
    sleep(5);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Create and start a test eCall.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_ecall_StartTest
(
    void
)
{
    le_ecall_State_t state;

    LE_INFO("Start Testle_ecall_StartTest");

    LE_ASSERT(le_ecall_SetPsapNumber(PsapNumber) == LE_OK);

    LE_ASSERT(le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PUSH) == LE_OK);

    LE_ASSERT((LastTestECallRef=le_ecall_Create()) != NULL);

    LE_ASSERT(le_ecall_SetMsdPosition(LastTestECallRef, true, +48898064, +2218092, 0) == LE_OK);
    LE_ASSERT(le_ecall_SetMsdPositionN1(LastTestECallRef, 11, -22) == LE_OK);
    LE_ASSERT(le_ecall_SetMsdPositionN2(LastTestECallRef, -33, 44) == LE_OK);

    LE_ASSERT(le_ecall_SetMsdPassengersCount(LastTestECallRef, 3) == LE_OK);

    LE_ASSERT(le_ecall_StartTest(LastTestECallRef) == LE_OK);

    LE_ASSERT(le_ecall_StartManual(LastTestECallRef) == LE_BUSY);
    LE_ASSERT(le_ecall_StartAutomatic(LastTestECallRef) == LE_BUSY);

    state=le_ecall_GetState(LastTestECallRef);
    LE_ASSERT(((state>=LE_ECALL_STATE_STARTED) && (state<=LE_ECALL_STATE_FAILED)));
}

//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGINT/SIGTERM when process dies.
 */
//--------------------------------------------------------------------------------------------------
static void SigHandler
(
    int sigNum
)
{
    LE_INFO("End and delete last test eCall");
    le_ecall_End(LastTestECallRef);
    le_ecall_Delete(LastTestECallRef);
    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Add State Change Handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void* Test_thread(void* context)
{

    le_ecall_StateChangeHandlerRef_t   stateChangeHandlerRef = 0x00;

    le_ecall_ConnectService();


    LE_INFO("Add State Change Handler");
    LE_ASSERT((stateChangeHandlerRef =
         le_ecall_AddStateChangeHandler(MyECallEventHandler, NULL)) != NULL);

    LE_INFO("No event loop");
    le_event_RunLoop();

}

//--------------------------------------------------------------------------------------------------
/**
 * Helper.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage()
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
            "Usage of the eCallTest bin is:",
            "   app runProc eCallTest --exe=eCallTest -- <PSAP number>",
    };

    for(idx = 0; idx < NUM_ARRAY_MEMBERS(usagePtr); idx++)
    {
        if(sandboxed)
        {
            LE_INFO("%s", usagePtr[idx]);
        }
        else
        {
            fprintf(stderr, "%s\n", usagePtr[idx]);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    if (le_arg_NumArgs() == 1)
    {
        le_ecall_SystemStandard_t systemStandard = LE_ECALL_PAN_EUROPEAN;
        bool isEraGlonass = false;

        // Register a signal event handler for SIGINT when user interrupts/terminates process
        signal(SIGINT, SigHandler);

        PsapNumber = le_arg_GetArg(0);
        LE_INFO("======== Start eCall Modem Services implementation Test with PSAP.%s========",
                PsapNumber);

        // Add State Change Handler
        le_thread_Start(le_thread_Create("TestThread",Test_thread,NULL));

        // Get system standard
        if (LE_OK != le_ecall_GetSystemStandard(&systemStandard))
        {
            LE_FATAL("ERROR le_ecall_GetSystemStandard failed.");
        }
        else
        {
            LE_INFO("le_ecall_SetSystemStandard %d!", systemStandard);
            if( LE_ECALL_ERA_GLONASS == systemStandard )
            {
                isEraGlonass = true;
            }
        }

        // Start Test
        LE_INFO("======== OperationMode Test  ========");
        Testle_ecall_OperationMode();
        LE_INFO("======== ConfigSettings Test  ========");
        Testle_ecall_ConfigSettings();
        if (isEraGlonass)
        {
            LE_INFO("Selected standard is ERA GLONASS");
            LE_INFO("======== EraGlonassSettings Test  ========");
            Testle_ecall_EraGlonassSettings();
        }
        else
        {
            LE_INFO("Selected standard is PAN EUROPEAN, EraGlonassSettings test is not ran.");
        }
        LE_INFO("======== LoadMsd Test  ========");
        Testle_ecall_LoadMsd();
        LE_INFO("======== StartManual Test  ========");
        Testle_ecall_StartManual();
        LE_INFO("======== StartTest Test  ========");
        Testle_ecall_StartTest();
        LE_INFO("======== Test eCall Modem Services implementation Test SUCCESS ========");
        exit(0);
    }
    else
    {
        PrintUsage();
        LE_INFO("EXIT eCallTest");
        exit(EXIT_FAILURE);
    }
}
