 /**
  * This module is for unit testing of the Positioning component.
  *
  * Run cmake -DENABLE_SIMUL=1 before launching make if you want to use the simulator.
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <semaphore.h>
#include <sys/ioctl.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>

#include "main.h"

#ifdef ENABLE_SIMUL
#include "atMgr.h"
#include "atCmdSync.h"
#include "atPorts.h"
#include "atPortsInternal.h"
#include "atMachineDevice.h"
#endif

#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Main.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* test(void* context)
{
    // Init the test case / test suite data structures
    CU_TestInfo postest[] =
    {
        { "Test le_pos_Fix()",         Testle_pos_Fix },
        { "Test le_pos_Navigation()",  Testle_pos_Navigation },
        CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
        { "POS tests",                NULL, NULL, postest },
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
    }

    le_event_RunLoop();
//     CU_cleanup_registry();
//     exit(CU_get_error());
}


#ifdef ENABLE_SIMUL

#define CUSTOM_PORT     "/tmp/modem_gnss"

static int thisopen(const char* path)
{
    bool isOpen=false;
    int sockfd,i;
    struct sockaddr_un that;
    memset(&that,0,sizeof(that)); /* System V */
    that.sun_family = AF_UNIX;
    strcpy(that.sun_path,path);

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        LE_FATAL("socket failed (%d):%s",errno,strerror(errno));
    }

    for(i=0;(i<60)&&(!isOpen);i++)
    {
        if (connect(sockfd, (struct sockaddr* )&that, sizeof(that)) < 0)  /* error */
        {
            LE_WARN("[%i] connect to '%s' failed (%d):%s",i,CUSTOM_PORT,errno,strerror(errno));
            sleep(1);
        } else {
            isOpen=true;
        }
    }

    if (isOpen) {
        LE_INFO("Connection to socket '%s' is done",CUSTOM_PORT);
    } else {
        LE_FATAL("Cannot connect to socket '%s'",CUSTOM_PORT);
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

static void CreateAtPortCommand
(
    void
)
{
    struct atdevice atDevice;

    memset(&atDevice,0,sizeof(atDevice));

    le_utf8_Copy(atDevice.name,"ATCUSTOM",sizeof(atDevice.name),0);
    le_utf8_Copy(atDevice.path,CUSTOM_PORT,sizeof(atDevice.path),0);
    atDevice.deviceItf.open = (le_da_DeviceOpenFunc_t)thisopen;
    atDevice.deviceItf.read = (le_da_DeviceReadFunc_t)thisread;
    atDevice.deviceItf.write = (le_da_DeviceWriteFunc_t)thiswrite;
    atDevice.deviceItf.io_control = (le_da_DeviceIoControlFunc_t)thisioctl;
    atDevice.deviceItf.close = (le_da_DeviceCloseFunc_t)thisclose;

    atmgr_Ref_t interfacePtr = atmgr_CreateInterface(&atDevice);

    atports_SetInterface(ATPORT_COMMAND,interfacePtr);

    LE_FATAL_IF(atports_GetInterface(ATPORT_COMMAND)==NULL,"Could not create the interface");

    LE_DEBUG("Port %s [%s] is created","ATCUSTOM",CUSTOM_PORT);
}

#endif


COMPONENT_INIT
{
#ifdef ENABLE_SIMUL

    atmgr_Start();

    atcmdsync_Init();

    CreateAtPortCommand();

#endif

    LE_ASSERT(le_posCtrl_Request() != NULL);

    le_thread_Start(le_thread_Create("POSTest",test,NULL));
}
