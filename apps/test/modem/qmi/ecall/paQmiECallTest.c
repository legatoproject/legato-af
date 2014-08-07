/** @file paQmiECallTest.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */


#include "legato.h"
#include "pa.h"
#include "pa_ecall_qmi.c"

static char PSAP_TEL_1[] = "112";
static char PSAP_TEL_2[] = "";
static char PSAP_TEL_3[] = "123456789012345";

#define LE_PRINT_VALUE(formatSpec, value)                                                         \
    LE_INFO( STRINGIZE(value) "=" formatSpec, value)

// Example of imported MSD:
// {
//   msdVersion 2,
//   msd CONTAINING {
//     msdStructure {
//       messageIdentifier 1,
//       control {
//         automaticActivation TRUE,
//         testCall FALSE,
//         positionCanBeTrusted TRUE,
//         vehicleType passengerVehicleClassM1
//       },
//       vehicleIdentificationNumber {
//         isowmi "ECA",
//         isovds "LLEXAM",
//         isovisModelyear "P",
//         isovisSeqPlant "LE02013"
//       },
//       vehiclePropulsionStorageType {
//         gasolineTankPresent TRUE,
//         dieselTankPresent FALSE,
//         compressedNaturalGas FALSE,
//         liquidPropaneGas FALSE,
//         electricEnergyStorage FALSE,
//         hydrogenStorage FALSE,
//         otherStorage FALSE
//       },
//       timestamp 1367878452,
//       vehicleLocation {
//         positionLatitude 18859320,
//         positionLongitude 187996428
//       },
//       vehicleDirection 45,
//       recentVehicleLocationN1 {
//         latitudeDelta 0,
//         longitudeDelta 10
//       },
//       recentVehicleLocationN2 {
//         latitudeDelta 0,
//         longitudeDelta 30
//       },
//       numberOfPassengers 2
//     }
//   }
// }

static uint8_t MsdBlob_1[] ={0x02, 0x25, 0x1C, 0x06, 0x80, 0xE3, 0x0A, 0x51, 0x43, 0x9E, 0x29, 0x55,
                             0xD4, 0x38, 0x00, 0x80, 0x04, 0x37, 0xF8, 0x0A, 0x31, 0x05, 0x66, 0x90,
                             0x23, 0xF8, 0xA7, 0x11, 0x66, 0x93, 0x21, 0x85, 0xB0, 0x04, 0x15, 0x00,
                             0x43, 0xC0, 0x40};

#define MSDBLOB_1 "\"" \
                  "02251C0680E30A51439E2955" \
                  "D43800800437F80A31056690" \
                  "23F8A71166932185B0041500" \
                  "43C040" \
                  "\""


static void PrintConfiguration
(
    swi_m2m_ecall_config_req_msg_v01* configurationPtr
)
{
    LE_INFO("QMI Configuration:");
    LE_PRINT_VALUE("%d",configurationPtr->configure_eCall_session.dial_type);
    LE_PRINT_VALUE("%d",configurationPtr->configure_eCall_session.host_build_msd);
    LE_PRINT_VALUE("%d",configurationPtr->configure_eCall_session.voc_mode);
    LE_PRINT_VALUE("%d",configurationPtr->modem_msd_type);
    LE_PRINT_VALUE("%d",configurationPtr->num_valid);
    LE_PRINT_VALUE("%d",configurationPtr->num_len);
    LE_INFO("configurationPtr->num = %.32s",configurationPtr->num);
    LE_PRINT_VALUE("%d",configurationPtr->max_redial_attempt_valid);
    LE_PRINT_VALUE("%d",configurationPtr->max_redial_attempt);
    LE_PRINT_VALUE("%d",configurationPtr->gnss_update_time_valid);
    LE_PRINT_VALUE("%d",configurationPtr->gnss_update_time);
    LE_INFO("------------------");
}

static void PrintMsd
(
    swi_m2m_ecall_send_msd_blob_req_msg_v01* msdPtr
)
{
    LE_INFO("QMI MSD:");
    LE_PRINT_VALUE("%d",msdPtr->msd_blob_len);
    LE_INFO("msd blob = %.282s",msdPtr->msd_blob);
    LE_INFO("------------------");
}

static void PrintInitiatState
(
    void
)
{
    PrintConfiguration(&ECallConfiguration);
    PrintMsd(&ECallMsd);
}

static void Test_pa_ecall_SetPsapNumber
(
    void
)
{
    int i;
    memset(ECallConfiguration.num,0,sizeof(ECallConfiguration.num));
    LE_ASSERT(pa_ecall_SetPsapNumber(PSAP_TEL_1) == LE_OK);
    PrintConfiguration(&ECallConfiguration);
    LE_ASSERT(ECallConfiguration.num_valid == true);
    LE_ASSERT(ECallConfiguration.num_len == strlen(PSAP_TEL_1)+3);
    for(i=0;i<sizeof(ECallConfiguration.num);i++)
    {
        if ((i==0) || (i==strlen(PSAP_TEL_1)+1))
        {
            LE_ASSERT(ECallConfiguration.num[i] == '"');
        }
        else if (i<strlen(PSAP_TEL_1)+1)
        {
            LE_ASSERT(ECallConfiguration.num[i] == PSAP_TEL_1[i-1]);
        }
        else if (i==strlen(PSAP_TEL_1)+2)
        {
            LE_ASSERT(ECallConfiguration.num[i] == '\0');
        }
        else
        {
            LE_ASSERT(ECallConfiguration.num[i] == 0 );
        }
    }

    memset(ECallConfiguration.num,0,sizeof(ECallConfiguration.num));
    LE_ASSERT(pa_ecall_SetPsapNumber(PSAP_TEL_2) == LE_OK);
    PrintConfiguration(&ECallConfiguration);
    LE_ASSERT(ECallConfiguration.num_valid == true);
    LE_ASSERT(ECallConfiguration.num_len == strlen(PSAP_TEL_2)+3);
    for(i=0;i<sizeof(ECallConfiguration.num);i++)
    {
        if ((i==0) || (i==strlen(PSAP_TEL_2)+1))
        {
            LE_ASSERT(ECallConfiguration.num[i] == '"');
        }
        else if (i<strlen(PSAP_TEL_2)+1)
        {
            LE_ASSERT(ECallConfiguration.num[i] == PSAP_TEL_2[i-1]);
        }
        else if (i==strlen(PSAP_TEL_2)+2)
        {
            LE_ASSERT(ECallConfiguration.num[i] == '\0');
        }
        else
        {
            LE_ASSERT(ECallConfiguration.num[i] == 0 );
        }
    }

    memset(ECallConfiguration.num,0,sizeof(ECallConfiguration.num));
    LE_ASSERT(pa_ecall_SetPsapNumber(PSAP_TEL_3) == LE_OK);
    PrintConfiguration(&ECallConfiguration);
    LE_ASSERT(ECallConfiguration.num_valid == true);
    LE_ASSERT(ECallConfiguration.num_len == strlen(PSAP_TEL_3)+3);
    for(i=0;i<sizeof(ECallConfiguration.num);i++)
    {
        if ((i==0) || (i==strlen(PSAP_TEL_3)+1))
        {
            LE_ASSERT(ECallConfiguration.num[i] == '"');
        }
        else if (i<strlen(PSAP_TEL_3)+1)
        {
            LE_ASSERT(ECallConfiguration.num[i] == PSAP_TEL_3[i-1]);
        }
        else if (i==strlen(PSAP_TEL_3)+2)
        {
            LE_ASSERT(ECallConfiguration.num[i] == '\0');
        }
        else
        {
            LE_ASSERT(ECallConfiguration.num[i] == 0 );
        }
    }
}

static void Test_pa_ecall_SetMaxRedialAttempts
(
    void
)
{

    LE_ASSERT(pa_ecall_SetMaxRedialAttempts(5) == LE_OK);
    PrintConfiguration(&ECallConfiguration);
    LE_ASSERT(ECallConfiguration.max_redial_attempt_valid == true);
    LE_ASSERT(ECallConfiguration.max_redial_attempt == 5);

    LE_ASSERT(pa_ecall_SetMaxRedialAttempts(255) == LE_OK);
    PrintConfiguration(&ECallConfiguration);
    LE_ASSERT(ECallConfiguration.max_redial_attempt_valid == true);
    LE_ASSERT(ECallConfiguration.max_redial_attempt == 10);

    LE_ASSERT(pa_ecall_SetMaxRedialAttempts(1000) == LE_OK);
    PrintConfiguration(&ECallConfiguration);
    LE_ASSERT(ECallConfiguration.max_redial_attempt_valid == true);
    LE_ASSERT(ECallConfiguration.max_redial_attempt == 10);

    LE_ASSERT(pa_ecall_SetMaxRedialAttempts(0) == LE_OK);
    PrintConfiguration(&ECallConfiguration);
    LE_ASSERT(ECallConfiguration.max_redial_attempt_valid == true);
    LE_ASSERT(ECallConfiguration.max_redial_attempt == 0);
}

static void Test_pa_ecall_SetMsdTxMode
(
    void
)
{
    LE_ASSERT(pa_ecall_SetMsdTxMode(PA_ECALL_TX_MODE_PULL) == LE_OK);

    LE_ASSERT(pa_ecall_SetMsdTxMode(PA_ECALL_TX_MODE_PUSH) == LE_OK);
}

static void Test_pa_ecall_LoadMsd
(
    void
)
{
    int i=0;
    LE_ASSERT(pa_ecall_LoadMsd(MsdBlob_1,sizeof(MsdBlob_1)) == LE_OK);
    PrintMsd(&ECallMsd);

    for (i=0;i<sizeof(MSDBLOB_1);i++)
    {
        LE_ASSERT(ECallMsd.msd_blob[i]==MSDBLOB_1[i]);
    }
    LE_ASSERT(ECallMsd.msd_blob_len == strlen(MSDBLOB_1));
}

static void Test_Ping
(
    void
)
{
    qmi_client_error_type rc;

    swi_m2m_ping_req_msg_v01 pingReq = { {0} };
    swi_m2m_ping_resp_msg_v01 pingResp = { {0} };

    LE_INFO("ping %.4s",pingReq.ping);

    rc = qmi_client_send_msg_sync(MgsClient,
                                  QMI_SWI_M2M_PING_REQ_V01,
                                  &pingReq, sizeof(pingReq),
                                  &pingResp, sizeof(pingResp),
                                  COMM_TIMEOUT_MS);

    LE_ASSERT(CheckResponseCode(STRINGIZE_EXPAND(QMI_SWI_M2M_PING_REQ_V01),
                                rc, pingResp.resp) == LE_OK);

    LE_INFO("pong %.4s",pingResp.pong);
}

COMPONENT_INIT
{
    LE_INFO("======== Begin ECall Platform Adapter's QMI implementation Test  ========");

    LE_ASSERT(swiQmi_InitServices(QMI_SERVICE_MGS) == LE_OK);
    LE_ASSERT(ecall_qmi_Init() == LE_OK);

    Test_Ping();

    PrintInitiatState();

    LE_INFO("======== SetPsapNumber Test  ========");
    Test_pa_ecall_SetPsapNumber();
    LE_INFO("======== SetMaxRedialAttempts Test  ========");
    Test_pa_ecall_SetMaxRedialAttempts();
    LE_INFO("======== SetMsdTxMode Test  ========");
    Test_pa_ecall_SetMsdTxMode();
    LE_INFO("======== LoadMsd Test  ========");
    Test_pa_ecall_LoadMsd();

    LE_INFO("======== Test ECall Platform Adapter's QMI implementation Test SUCCESS ========");
    exit(EXIT_SUCCESS);
}
