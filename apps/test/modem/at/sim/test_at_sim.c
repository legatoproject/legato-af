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
#include "pa_sim.c"

#define     CUSTOM_PORT     "/tmp/modem_sim"

static void* pa_init(void* context)
{
    le_sem_Ref_t pSem = context;
    LE_INFO("Start PA");

    pa_sim_Init();

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

/* The suite cleanup function.
* Closes the temporary file used by the tests.
* Returns zero on success, non-zero otherwise.
*/
int clean_suite(void)
{
    return 0;
}

void test_CheckStatus_CmsErrorCode()
{
    le_sim_States_t state = LE_SIM_STATE_UNKNOWN;

    CheckStatus_CmsErrorCode("1",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);

    CheckStatus_CmsErrorCode("515",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_BUSY);

    CheckStatus_CmsErrorCode("318",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    CheckStatus_CmsErrorCode("317",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    CheckStatus_CmsErrorCode("316",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    CheckStatus_CmsErrorCode("313",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);

    CheckStatus_CmsErrorCode("312",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    CheckStatus_CmsErrorCode("311",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    CheckStatus_CmsErrorCode("310",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_ABSENT);
}

void test_CheckStatus_CmeErrorCode()
{
    le_sim_States_t state = LE_SIM_STATE_UNKNOWN;

    CheckStatus_CmeErrorCode("1",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);

    CheckStatus_CmeErrorCode("5",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    CheckStatus_CmeErrorCode("11",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    CheckStatus_CmeErrorCode("16",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    CheckStatus_CmeErrorCode("17",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    CheckStatus_CmeErrorCode("10",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_ABSENT);

    CheckStatus_CmeErrorCode("12",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    CheckStatus_CmeErrorCode("18",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    CheckStatus_CmeErrorCode("3",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);

    CheckStatus_CmeErrorCode("4",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);

    CheckStatus_CmeErrorCode("13",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);
}

void test_pa_sim_ChecktStatus_CpinCode()
{
    le_sim_States_t state = LE_SIM_STATE_UNKNOWN;

    CheckStatus_CmsErrorCode("READY",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_READY);

    CheckStatus_CmsErrorCode("SIM PIN",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    CheckStatus_CmsErrorCode("SIM PUK",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    CheckStatus_CmsErrorCode("PH-SIM PIN",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    CheckStatus_CmsErrorCode("SIM PUK2",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    CheckStatus_CmsErrorCode("SIM PIN2",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    CheckStatus_CmsErrorCode("YOUHOU",&state);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);
}

void test_CheckStatus()
{
    bool result;
    le_sim_States_t state = LE_SIM_STATE_UNKNOWN;

    result = CheckStatus("OK",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_READY);

    result = CheckStatus("ERROR",&state);
    CU_ASSERT_EQUAL(result,false);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);

    result = CheckStatus("+CMS ERROR: 1",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);

    result = CheckStatus("+CMS ERROR: 515",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_BUSY);

    result = CheckStatus("+CMS ERROR: 318",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    result = CheckStatus("+CMS ERROR: 317",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = CheckStatus("+CMS ERROR: 316",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    result = CheckStatus("+CMS ERROR: 313",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);

    result = CheckStatus("+CMS ERROR: 312",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = CheckStatus("+CMS ERROR: 311",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = CheckStatus("+CMS ERROR: 310",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_ABSENT);

    result = CheckStatus("+CME ERROR: 1",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);

    result = CheckStatus("+CME ERROR: 5",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = CheckStatus("+CME ERROR: 11",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = CheckStatus("+CME ERROR: 16",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = CheckStatus("+CME ERROR: 17",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = CheckStatus("+CME ERROR: 10",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_ABSENT);

    result = CheckStatus("+CME ERROR: 12",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    result = CheckStatus("+CME ERROR: 18",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    result = CheckStatus("+CME ERROR: 3",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);

    result = CheckStatus("+CME ERROR: 4",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);

    result = CheckStatus("+CME ERROR: 13",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);

    result = CheckStatus("+CPIN: READY",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_READY);

    result = CheckStatus("+CPIN: SIM PIN",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = CheckStatus("+CPIN: SIM PUK",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    result = CheckStatus("+CPIN: PH-SIM PIN",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = CheckStatus("+CPIN: SIM PUK2",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    result = CheckStatus("+CPIN: SIM PIN2",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = CheckStatus("+CPIN: YOUHOU",&state);
    CU_ASSERT_EQUAL(result,true);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);
}

void testle_pasim_GetCardIdentification()
{
    le_result_t result;
    pa_sim_CardId_t cardId;

    result = pa_sim_GetCardIdentification(cardId);
    CU_ASSERT_EQUAL(result,LE_OK);
    fprintf(stdout,"%d cardId %s\n",result,cardId);

    CU_ASSERT_EQUAL(strlen(cardId),sizeof(cardId)-1);

    result = pa_sim_GetCardIdentification(cardId);
    CU_ASSERT_EQUAL(result,LE_OK);
    fprintf(stdout,"%d cardId %s\n",result,cardId);

    CU_ASSERT_EQUAL(strlen(cardId),sizeof(cardId)-1);
}

void testle_pasim_GetIMSI()
{
    le_result_t result;
    pa_sim_Imsi_t imsi;

    result = pa_sim_GetIMSI(imsi);
    CU_ASSERT_EQUAL(result,LE_OK);
    fprintf(stdout,"%d IMSI %s\n",result,imsi);

    CU_ASSERT_EQUAL(strlen(imsi),sizeof(imsi)-1);
}

void test_pa_sim_GetState()
{
    le_result_t result;
    le_sim_States_t state;

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_FAULT);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_READY);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_ABSENT);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_INSERTED);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_BLOCKED);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_STATE_UNKNOWN);

    result = pa_sim_GetState(&state);
    CU_ASSERT_EQUAL(result, LE_OK);
    CU_ASSERT_EQUAL(state,LE_SIM_BUSY);
}

static le_event_HandlerRef_t pHandler=NULL;
static void pa_sim_NewStateHdlrFunc(pa_sim_Event_t* event)
{
    le_mem_Release(event);
}

void test_pa_sim_AddNewStateHandler()
{
    CU_ASSERT_PTR_NULL(pHandler);
    pHandler = pa_sim_AddNewStateHandler(pa_sim_NewStateHdlrFunc);
    CU_ASSERT_PTR_NOT_NULL(pHandler);
}

void test_pa_sim_RemoveNewStateHandler()
{
    le_result_t result = pa_sim_RemoveNewStateHandler(pHandler);

    CU_ASSERT_EQUAL(result,LE_OK);
}

void test_pa_sim_EnterPIN()
{
    le_result_t result;
    result = pa_sim_EnterPIN(PA_SIM_PIN,"0000");
    CU_ASSERT_EQUAL(result,LE_FAULT);

    result = pa_sim_EnterPIN(PA_SIM_PIN,"1234");
    CU_ASSERT_EQUAL(result,LE_OK);
}

void test_pa_sim_EnterPUK()
{
    le_result_t result;
    result = pa_sim_EnterPUK(PA_SIM_PUK,"00000000","0000");
    CU_ASSERT_EQUAL(result,LE_FAULT);

    result = pa_sim_EnterPUK(PA_SIM_PUK,"00000000","1234");
    CU_ASSERT_EQUAL(result,LE_FAULT);

    result = pa_sim_EnterPUK(PA_SIM_PUK,"12345678","1234");
    CU_ASSERT_EQUAL(result,LE_OK);

    result = pa_sim_EnterPUK(PA_SIM_PUK,"12345678","0000");
    CU_ASSERT_EQUAL(result,LE_OK);
}

void test_pa_sim_GetPINRemainingAttempts()
{
    le_result_t result;
    uint32_t attemps;
    result = pa_sim_GetPINRemainingAttempts(PA_SIM_PIN,&attemps);
    CU_ASSERT_EQUAL(result,LE_FAULT);

    result = pa_sim_GetPINRemainingAttempts(PA_SIM_PIN,&attemps);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(attemps,3);

    result = pa_sim_GetPINRemainingAttempts(PA_SIM_PIN2,&attemps);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(attemps,2);
}

void test_pa_sim_GetPUKRemainingAttempts()
{
    le_result_t result;
    uint32_t attemps;
    result = pa_sim_GetPUKRemainingAttempts(PA_SIM_PUK,&attemps);
    CU_ASSERT_EQUAL(result,LE_FAULT);

    result = pa_sim_GetPUKRemainingAttempts(PA_SIM_PUK,&attemps);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(attemps,1);

    result = pa_sim_GetPUKRemainingAttempts(PA_SIM_PUK2,&attemps);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(attemps,0);
}

void test_pa_sim_ChangePIN()
{
    le_result_t result;
    result = pa_sim_ChangePIN(PA_SIM_PIN,"11111111","1111");
    CU_ASSERT_EQUAL(result,LE_OK);

    result = pa_sim_ChangePIN(PA_SIM_PIN2,"22222222","2222");
    CU_ASSERT_EQUAL(result,LE_OK);
}

void test_pa_sim_EnablePIN()
{
    le_result_t result;
    result = pa_sim_EnablePIN(PA_SIM_PIN,"1111");
    CU_ASSERT_EQUAL(result,LE_OK);

    result = pa_sim_EnablePIN(PA_SIM_PIN2,"2222");
    CU_ASSERT_EQUAL(result,LE_OK);
}

void test_pa_sim_DisablePIN()
{
    le_result_t result;
    result = pa_sim_DisablePIN(PA_SIM_PIN,"1111");
    CU_ASSERT_EQUAL(result,LE_OK);

    result = pa_sim_DisablePIN(PA_SIM_PIN2,"2222");
    CU_ASSERT_EQUAL(result,LE_OK);
}

static void* simtest(void* context)
{
    // Init the test case / test suite data structures

    CU_TestInfo test[] =
    {
        { "Test CheckStatus_CmsErrorCode", test_CheckStatus_CmsErrorCode },
        { "Test CheckStatus_CmeErrorCode", test_CheckStatus_CmeErrorCode },
        { "Test CheckStatus", test_CheckStatus },
        { "Test pa_sim_GetCardIdentification", testle_pasim_GetCardIdentification },
        { "Test pa_sim_GetIMSI", testle_pasim_GetIMSI },
        { "Test pa_sim_GetState", test_pa_sim_GetState },
        { "Test pa_sim_AddNewStateHandler", test_pa_sim_AddNewStateHandler },
        { "Test pa_sim_RemoveNewStateHandler", test_pa_sim_RemoveNewStateHandler },
        { "Test pa_sim_EnterPIN", test_pa_sim_EnterPIN },
        { "Test pa_sim_EnterPUK", test_pa_sim_EnterPUK },
        { "Test pa_sim_GetPINRemainingAttempts", test_pa_sim_GetPINRemainingAttempts },
        { "Test pa_sim_GetPUKRemainingAttempts", test_pa_sim_GetPUKRemainingAttempts },
        { "Test pa_sim_ChangePIN", test_pa_sim_ChangePIN },
        { "Test pa_sim_EnablePIN", test_pa_sim_EnablePIN },
        { "Test pa_sim_DisablePIN", test_pa_sim_DisablePIN },
        CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
        { "pa sim tests",      init_suite, clean_suite, test },
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

    le_thread_Start(le_thread_Create("ATSimTest",simtest,NULL));
}

COMPONENT_INIT
{
    init();
}
