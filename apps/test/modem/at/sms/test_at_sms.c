/**
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>


// Header files for CUnit
#include "Console.h"
#include <Basic.h>

#include "le_da.h"
#include "le_uart.h"
#include "atMgr.h"

#include "atCmdSync.h"
#include "atPorts.h"
#include "atPortsInternal.h"
#include "atMachineDevice.h"
#include "pa_sms.c"

#define     CUSTOM_PORT     "/tmp/modem_sms"

static void* pa_init(void* context)
{
    le_sem_Ref_t pSem = context;
    LE_INFO("Start PA");

    pa_sms_Init();

    le_sem_Post(pSem);
    le_event_RunLoop();
    return NULL;
}

static int thisopen (const char* path)
{
    int sockfd;
    struct sockaddr_un that;
    memset(&that,0,sizeof(that)); /* en System V */
    that.sun_family = AF_UNIX;
    strcpy(that.sun_path,path);

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return sockfd;
    }

    if (connect(sockfd, (struct sockaddr* )&that, sizeof(that)) < 0) /* error */
    {
        perror("connect");
        close(sockfd);
        sockfd = -1;
    }

    return sockfd;
}

static int thiswrite (uint32_t Handle, const char* buf, uint32_t buf_len)
{
    LE_FATAL_IF(Handle==-1,"Write Handle error\n");

    return write(Handle, buf, buf_len);
}

static int thisread (uint32_t Handle, char* buf, uint32_t buf_len)
{
    LE_FATAL_IF(Handle==-1,"Read Handle error\n");

    return read(Handle, buf, buf_len);
}

static int thisioctl   (uint32_t Handle, uint32_t Cmd, void* pParam )
{
    LE_FATAL_IF(Handle==-1,"ioctl Handle error\n");

    return ioctl(Handle, Cmd, pParam);
}

static int thisclose (uint32_t Handle)
{
    LE_FATAL_IF(Handle==-1,"close Handle error\n");

    return close(Handle);
}

static void OpenAtDeviceCommunication
(
    void
)
{
    struct atdevice atDevice;

    memset(&atDevice,0,sizeof(atDevice));

    le_utf8_Copy(atDevice.name,"CUSTOM",sizeof(atDevice.name),0);
    le_utf8_Copy(atDevice.path,CUSTOM_PORT,sizeof(atDevice.path),0);
    atDevice.deviceItf.open = (le_da_DeviceOpenFunc_t)thisopen;
    atDevice.deviceItf.read = (le_da_DeviceReadFunc_t)thisread;
    atDevice.deviceItf.write = (le_da_DeviceWriteFunc_t)thiswrite;
    atDevice.deviceItf.io_control = (le_da_DeviceIoControlFunc_t)thisioctl;
    atDevice.deviceItf.close = (le_da_DeviceCloseFunc_t)thisclose;

    atports_SetInterface(ATPORT_COMMAND,atmgr_CreateInterface(&atDevice));
}

/* The suite initialization function.
* Opens the temporary file used by the tests.
* Returns zero on success, non-zero otherwise.
*/
int init_suite(void)
{
    return 0;
}

static void pa_sms_NewMsgHdlrFunc(pa_sms_NewMessageIndication_t* msgRef)
{

}

void test_pa_sms_SetNewMsgHandler()
{
    le_result_t result = pa_sms_SetNewMsgHandler(pa_sms_NewMsgHdlrFunc);
    CU_ASSERT_EQUAL(result,LE_OK);
}

void test_pa_sms_ClearNewMsgHandler()
{
    le_result_t result = pa_sms_ClearNewMsgHandler();
    CU_ASSERT_EQUAL(result,LE_OK);
}

void test_SetNewMsgIndicLocal()
{
    SetNewMsgIndicLocal(PA_SMS_MT_0,PA_SMS_BM_0,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_0,PA_SMS_BM_0,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_0,PA_SMS_BM_0,PA_SMS_DS_2);
    SetNewMsgIndicLocal(PA_SMS_MT_0,PA_SMS_BM_1,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_0,PA_SMS_BM_1,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_0,PA_SMS_BM_1,PA_SMS_DS_2);
    SetNewMsgIndicLocal(PA_SMS_MT_0,PA_SMS_BM_2,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_0,PA_SMS_BM_2,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_0,PA_SMS_BM_2,PA_SMS_DS_2);
    SetNewMsgIndicLocal(PA_SMS_MT_0,PA_SMS_BM_3,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_0,PA_SMS_BM_3,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_0,PA_SMS_BM_3,PA_SMS_DS_2);

    SetNewMsgIndicLocal(PA_SMS_MT_1,PA_SMS_BM_0,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_1,PA_SMS_BM_0,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_1,PA_SMS_BM_0,PA_SMS_DS_2);
    SetNewMsgIndicLocal(PA_SMS_MT_1,PA_SMS_BM_1,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_1,PA_SMS_BM_1,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_1,PA_SMS_BM_1,PA_SMS_DS_2);
    SetNewMsgIndicLocal(PA_SMS_MT_1,PA_SMS_BM_2,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_1,PA_SMS_BM_2,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_1,PA_SMS_BM_2,PA_SMS_DS_2);
    SetNewMsgIndicLocal(PA_SMS_MT_1,PA_SMS_BM_3,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_1,PA_SMS_BM_3,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_1,PA_SMS_BM_3,PA_SMS_DS_2);

    SetNewMsgIndicLocal(PA_SMS_MT_2,PA_SMS_BM_0,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_2,PA_SMS_BM_0,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_2,PA_SMS_BM_0,PA_SMS_DS_2);
    SetNewMsgIndicLocal(PA_SMS_MT_2,PA_SMS_BM_1,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_2,PA_SMS_BM_1,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_2,PA_SMS_BM_1,PA_SMS_DS_2);
    SetNewMsgIndicLocal(PA_SMS_MT_2,PA_SMS_BM_2,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_2,PA_SMS_BM_2,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_2,PA_SMS_BM_2,PA_SMS_DS_2);
    SetNewMsgIndicLocal(PA_SMS_MT_2,PA_SMS_BM_3,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_2,PA_SMS_BM_3,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_2,PA_SMS_BM_3,PA_SMS_DS_2);

    SetNewMsgIndicLocal(PA_SMS_MT_3,PA_SMS_BM_0,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_3,PA_SMS_BM_0,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_3,PA_SMS_BM_0,PA_SMS_DS_2);
    SetNewMsgIndicLocal(PA_SMS_MT_3,PA_SMS_BM_1,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_3,PA_SMS_BM_1,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_3,PA_SMS_BM_1,PA_SMS_DS_2);
    SetNewMsgIndicLocal(PA_SMS_MT_3,PA_SMS_BM_2,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_3,PA_SMS_BM_2,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_3,PA_SMS_BM_2,PA_SMS_DS_2);
    SetNewMsgIndicLocal(PA_SMS_MT_3,PA_SMS_BM_3,PA_SMS_DS_0);
    SetNewMsgIndicLocal(PA_SMS_MT_3,PA_SMS_BM_3,PA_SMS_DS_1);
    SetNewMsgIndicLocal(PA_SMS_MT_3,PA_SMS_BM_3,PA_SMS_DS_2);
}

// TODO if needed, add all test
void test_pa_sms_SetNewMsgIndic()
{
    le_result_t result;

    result = pa_sms_SetNewMsgIndic(PA_SMS_NMI_MODE_0,PA_SMS_MT_0,PA_SMS_BM_0,PA_SMS_DS_0,PA_SMS_BFR_0);
    CU_ASSERT_EQUAL(result,LE_OK);
}

void test_pa_sms_GetNewMsgIndic()
{
    le_result_t result;
    pa_sms_NmiMode_t mode;
    pa_sms_NmiMt_t   mt;
    pa_sms_NmiBm_t   bm;
    pa_sms_NmiDs_t   ds;
    pa_sms_NmiBfr_t  bfr;

    result = pa_sms_GetNewMsgIndic(&mode,&mt,&bm,&ds,&bfr);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(mode,PA_SMS_NMI_MODE_0);
    CU_ASSERT_EQUAL(mt,PA_SMS_MT_1);
    CU_ASSERT_EQUAL(bm,PA_SMS_BM_0);
    CU_ASSERT_EQUAL(ds,PA_SMS_DS_1);
    CU_ASSERT_EQUAL(bfr,PA_SMS_BFR_0);
}

void test_pa_sms_SetMsgFormat()
{
    le_result_t result;

    result = pa_sms_SetMsgFormat(LE_SMS_FORMAT_PDU);
    CU_ASSERT_EQUAL(result,LE_OK);

    result = pa_sms_SetMsgFormat(LE_SMS_FORMAT_TEXT);
    CU_ASSERT_EQUAL(result,LE_OK);
}

#define MESSAGE_RCV_PDU_7BITS 0x07913366003001F0040B913366611568F600003140204155558011D4329E0EA296E774103C4CA797E56E
static uint8_t message[] =
{
    0x07, 0x91, 0x33, 0x66, 0x00, 0x30, 0x01, 0xF0,
    0x04, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
    0xF6, 0x00, 0x00, 0x31, 0x40, 0x20, 0x41, 0x55,
    0x55, 0x80, 0x11, 0xD4, 0x32, 0x9E, 0x0E, 0xA2,
    0x96, 0xE7, 0x74, 0x10, 0x3C, 0x4C, 0xA7, 0x97,
    0xE5, 0x6E
};

void test_pa_sms_SendPduMsg()
{
    uint32_t  msgref;
    pa_sms_SendingErrCode_t  errorCode;
    msgref = pa_sms_SendPduMsg(PA_SMS_PROTOCOL_GSM,29,message, PA_SMS_SENDING_TIMEOUT, &errorCode);
    CU_ASSERT_EQUAL(msgref,15);
}

void test_pa_sms_RdPDUMsgFromMem()
{
    le_result_t result;
    pa_sms_Pdu_t pdu;
    int32_t i;

    result = pa_sms_RdPDUMsgFromMem(1,PA_SMS_PROTOCOL_GSM,PA_SMS_STORAGE_SIM,&pdu);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(pdu.status,LE_SMS_RX_UNREAD);
    CU_ASSERT_EQUAL(pdu.dataLen,sizeof(message));
    for(i=0;i<pdu.dataLen;i++) {
        CU_ASSERT_EQUAL(pdu.data[i],message[i]);
    }
}

void test_pa_sms_ListMsgFromMem()
{
    le_result_t result;
    uint32_t  size;
    uint32_t  tab[99]={0};
    int32_t i;

    result = pa_sms_ListMsgFromMem(LE_SMS_RX_READ, PA_SMS_PROTOCOL_GSM, &size, tab, PA_SMS_STORAGE_SIM);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(size,0);

    result = pa_sms_ListMsgFromMem(LE_SMS_RX_READ, PA_SMS_PROTOCOL_GSM, &size, tab, PA_SMS_STORAGE_SIM);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(size,3);
    for(i=0;i<size;i++) {
        CU_ASSERT_NOT_EQUAL(tab[i],0);
    }

}

void test_pa_sms_DelMsgFromMem()
{
    le_result_t result;

    result = pa_sms_DelMsgFromMem(5,PA_SMS_PROTOCOL_GSM,PA_SMS_STORAGE_SIM);
    CU_ASSERT_EQUAL(result,LE_OK);
}

void test_pa_sms_DelAllMsg()
{
    le_result_t result;

    result = pa_sms_DelAllMsg();
    CU_ASSERT_EQUAL(result,LE_OK);
}

void test_pa_sms_SaveSettings()
{
    le_result_t result;

    result = pa_sms_SaveSettings();
    CU_ASSERT_EQUAL(result,LE_OK);
}

void test_pa_sms_RestoreSettings()
{
    le_result_t result;

    result = pa_sms_RestoreSettings();
    CU_ASSERT_EQUAL(result,LE_OK);
}


/* The suite cleanup function.
* Closes the temporary file used by the tests.
* Returns zero on success, non-zero otherwise.
*/
int clean_suite(void)
{
    return 0;
}

static void* smstest(void* context)
{
    // Init the test case / test suite data structures

    CU_TestInfo test[] =
    {
        { "Test pa_sms_SetNewMsgHandler", test_pa_sms_SetNewMsgHandler },
        { "Test pa_sms_ClearNewMsgHandler", test_pa_sms_ClearNewMsgHandler },
        { "Test SetNewMsgIndicLocal", test_SetNewMsgIndicLocal },
        { "Test pa_sms_SetNewMsgIndic", test_pa_sms_SetNewMsgIndic },
        { "Test pa_sms_GetNewMsgIndic", test_pa_sms_GetNewMsgIndic },
        { "Test pa_sms_SetMsgFormat", test_pa_sms_SetMsgFormat },
        { "Test pa_sms_SendPduMsg", test_pa_sms_SendPduMsg },
        { "Test pa_sms_RdPDUMsgFromMem", test_pa_sms_RdPDUMsgFromMem },
        { "Test pa_sms_ListMsgFromMem", test_pa_sms_ListMsgFromMem },
        { "Test pa_sms_DelMsgFromMem", test_pa_sms_DelMsgFromMem },
        { "Test pa_sms_DelAllMsg", test_pa_sms_DelAllMsg },
        { "Test pa_sms_SaveSettings", test_pa_sms_SaveSettings },
        { "Test pa_sms_RestoreSettings", test_pa_sms_RestoreSettings },
        CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
        { "pa sms tests",      init_suite, clean_suite, test },
        CU_SUITE_INFO_NULL,
    };

    // Initialize the CUnit test registry and register the test suite
    if (CUE_SUCCESS != CU_initialize_registry())
        exit(CU_get_error());

    if ( CUE_SUCCESS != CU_register_suites(suites))
    {
        CU_cleanup_registry();
        exit(CU_get_error());
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    // Output summary of failures, if there were any
    if ( CU_get_number_of_failures() > 0 )
    {
        fprintf(stdout,"\n [START]List of Failure :\n");
        CU_basic_show_failures(CU_get_failure_list());
        fprintf(stdout,"\n [STOP]List of Failure\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

int Exists(const char *fname)
{
    int file;
    file = thisopen(fname);
    if (file != -1)
    {
        thisclose(file);
        return 1;
    }
    return 0;
}

static void init()
{
    // Wait for CUSTOM_PORT to be available
    {
        int i;
        for (i=10;i>0;i--)
        {
            if (!Exists(CUSTOM_PORT))
            {
                int sec = 1;
                // Wait 1 seconds and retry
                fprintf(stdout,"%s does not exist, retry in %d sec\n",CUSTOM_PORT,sec);
                sleep(sec);
            }
            else
            {
                fprintf(stdout,"%s exist, can continue the test\n",CUSTOM_PORT);
                break;
            }
        }
        if (i==0)
        {
            exit(EXIT_FAILURE);
        }
    }

    atmgr_Start();

    atcmdsync_Init();

    OpenAtDeviceCommunication();

    atmgr_StartInterface(atports_GetInterface(ATPORT_COMMAND));

    le_sem_Ref_t pSem = le_sem_Create("PAStartSem",0);

    le_thread_Start(le_thread_Create("PA_TEST",pa_init,pSem));

    le_sem_Wait(pSem);
    LE_INFO("PA is started");
    le_sem_Delete(pSem);

    le_thread_Start(le_thread_Create("ATSmsTest",smstest,NULL));
}

COMPONENT_INIT
{
    init();
}
