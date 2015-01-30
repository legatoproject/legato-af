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
#include "pa_mrc.c"

#define     CUSTOM_PORT     "/tmp/modem_mrc"

static void* pa_init(void* context)
{
    le_sem_Ref_t pSem = context;
    LE_INFO("Start PA");

    pa_mrc_Init();

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

    le_utf8_Copy(atDevice.name,"CUSTOM_PORT",sizeof(atDevice.name),0);
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

void test_pa_mrc_SetRadioPower()
{
    le_result_t result;

    result = pa_mrc_SetRadioPower(LE_OFF);
    CU_ASSERT_EQUAL(result,LE_OK);

    result = pa_mrc_SetRadioPower(LE_ON);
    CU_ASSERT_EQUAL(result,LE_OK);
}

void test_pa_mrc_GetRadioPower()
{
    le_result_t result;
    le_onoff_t    power;

    result = pa_mrc_GetRadioPower(&power);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(power,LE_OFF);

    result = pa_mrc_GetRadioPower(&power);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(power,LE_ON);
}

static le_event_HandlerRef_t pHandler=NULL;
static void pa_mrc_NetworkRegHdlrFunc(le_mrc_NetRegState_t* regState)
{
}

void test_pa_mrc_AddNetworkRegHandler()
{
    CU_ASSERT_PTR_NULL(pHandler);
    pHandler = pa_mrc_AddNetworkRegHandler(pa_mrc_NetworkRegHdlrFunc);
    CU_ASSERT_PTR_NOT_NULL(pHandler);
}

void test_pa_mrc_RemoveNetworkRegHandler()
{
    le_result_t result = pa_mrc_RemoveNetworkRegHandler(pHandler);

    CU_ASSERT_EQUAL(result,LE_OK);
}

void test_SubscribeUnsolCREG()
{
    SubscribeUnsolCREG(PA_MRC_DISABLE_REG_NOTIFICATION);
    CU_ASSERT_EQUAL(ThisMode,PA_MRC_DISABLE_REG_NOTIFICATION);

    SubscribeUnsolCREG(PA_MRC_ENABLE_REG_NOTIFICATION);
    CU_ASSERT_EQUAL(ThisMode,PA_MRC_ENABLE_REG_NOTIFICATION);

    SubscribeUnsolCREG(PA_MRC_ENABLE_REG_LOC_NOTIFICATION);
    CU_ASSERT_EQUAL(ThisMode,PA_MRC_ENABLE_REG_LOC_NOTIFICATION);
}

void test_pa_mrc_ConfigureNetworkReg()
{
    le_result_t result;

    result = pa_mrc_ConfigureNetworkReg(PA_MRC_DISABLE_REG_NOTIFICATION);
    CU_ASSERT_EQUAL(result,LE_OK);

    result = pa_mrc_ConfigureNetworkReg(PA_MRC_ENABLE_REG_NOTIFICATION);
    CU_ASSERT_EQUAL(result,LE_OK);

    result = pa_mrc_ConfigureNetworkReg(PA_MRC_ENABLE_REG_LOC_NOTIFICATION);
    CU_ASSERT_EQUAL(result,LE_OK);

}

void testGetNetworkReg()
{
    le_result_t result;
    int32_t  value;

    result = GetNetworkReg(true,&value);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(value,1);

    result = GetNetworkReg(false,&value);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(value,2);
}

void test_pa_mrc_GetNetworkRegConfig()
{
    le_result_t result;
    pa_mrc_NetworkRegSetting_t  value;

    result = pa_mrc_GetNetworkRegConfig(&value);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(value,PA_MRC_DISABLE_REG_NOTIFICATION);

    result = pa_mrc_GetNetworkRegConfig(&value);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(value,PA_MRC_ENABLE_REG_NOTIFICATION);

    result = pa_mrc_GetNetworkRegConfig(&value);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(value,PA_MRC_ENABLE_REG_LOC_NOTIFICATION);
}

void test_pa_mrc_GetNetworkRegState()
{
    le_result_t result;
    le_mrc_NetRegState_t  value;

    result = pa_mrc_GetNetworkRegState(&value);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(value,LE_MRC_REG_NONE);

    result = pa_mrc_GetNetworkRegState(&value);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(value,LE_MRC_REG_HOME);

    result = pa_mrc_GetNetworkRegState(&value);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(value,LE_MRC_REG_SEARCHING);

    result = pa_mrc_GetNetworkRegState(&value);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(value,LE_MRC_REG_DENIED);

    result = pa_mrc_GetNetworkRegState(&value);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(value,LE_MRC_REG_UNKNOWN);

    result = pa_mrc_GetNetworkRegState(&value);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(value,LE_MRC_REG_ROAMING);
}

void test_pa_mrc_GetSignalStrength()
{
    le_result_t result;
    int32_t rssi=0;

    result = pa_mrc_GetSignalStrength(&rssi);
    CU_ASSERT_EQUAL(result,LE_OK);
    fprintf(stdout,"rssi %d\n",rssi);
    CU_ASSERT_EQUAL(rssi,-113);

    result = pa_mrc_GetSignalStrength(&rssi);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(rssi,-51);
}


static void* rctest(void* context)
{
    // Init the test case / test suite data structures

    CU_TestInfo test[] =
    {
        { "Test pa_mrc_SetRadioPower", test_pa_mrc_SetRadioPower },
        { "Test pa_mrc_GetRadioPower", test_pa_mrc_GetRadioPower },
        { "Test pa_mrc_AddNetworkRegHandler", test_pa_mrc_AddNetworkRegHandler },
        { "Test pa_mrc_RemoveNetworkRegHandler", test_pa_mrc_RemoveNetworkRegHandler },
        { "Test SubscribeUnsolCREG", test_SubscribeUnsolCREG },
        { "Test pa_mrc_ConfigureNetworkReg", test_pa_mrc_ConfigureNetworkReg },
        { "Test GetNetworkReg", testGetNetworkReg },
        { "Test pa_mrc_GetNetworkRegConfig", test_pa_mrc_GetNetworkRegConfig },
        { "Test pa_mrc_GetNetworkRegState", test_pa_mrc_GetNetworkRegState },
        { "Test pa_mrc_GetSignalStrength", test_pa_mrc_GetSignalStrength },
        CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
        { "pa rc tests",      init_suite, clean_suite, test },
        CU_SUITE_INFO_NULL,
    };

    // Initialize the CUnit test registry and register the test suite
    if (CUE_SUCCESS != CU_initialize_registry()) {
        fprintf(stdout,"ERROR CU_initialize_registry\n");
        exit(CU_get_error());
    }

    if ( CUE_SUCCESS != CU_register_suites(suites))
    {
        fprintf(stdout,"ERROR CU_register_suites\n");
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

    le_thread_Start(le_thread_Create("ATMrcTest",rctest,NULL));
}

COMPONENT_INIT
{
    init();
}
