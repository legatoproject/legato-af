/**
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <unistd.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>

#include "atMachineDevice.h"

#define     UART_PORT   "./test_le_atmgr_uart.log"

#define     BUFFERLENGTH    1024
#define     ATBUFFER        256

#define     WRITECMD    "TEST UART CMD WRITE\n"
#define     WRITEDATA   "TEST UART DATA WRITE\n"

static struct atdevice  uartDevice;

// Test this function :
// void  atmachinedevice_Write(atDevice_t *devicePtr,uint8_t *rxDataPtr,uint32_t size);
void testatmachinedevice_Write_cmd(void)
{
    lseek(uartDevice.handle,0,SEEK_SET);
    atmachinedevice_Write(&uartDevice,(uint8_t*)WRITECMD,strlen(WRITECMD));
    lseek(uartDevice.handle,0,SEEK_SET);

    CU_PASS("atmachinedevice_Write command");
}

// Test this function :
// void  atmachinedevice_Write(atDevice_t *devicePtr,uint8_t *rxDataPtr,uint32_t size);
void testatmachinedevice_Write_data(void)
{
    lseek(uartDevice.handle,0,SEEK_SET);
    atmachinedevice_Write(&uartDevice,(uint8_t*)WRITEDATA,strlen(WRITEDATA));
    lseek(uartDevice.handle,0,SEEK_SET);

    CU_PASS("atmachinedevice_Write data");
}

// Test this function :
// int32_t atmachinedevice_Read(atDevice_t *devicePtr,uint8_t *rxDataPtr,uint32_t size);
void testatmachinedevice_Read(void)
{
    uint32_t nb_read=0;
    char readbuffer[BUFFERLENGTH]={0};

    testatmachinedevice_Write_cmd();
    nb_read = atmachinedevice_Read(&uartDevice,(uint8_t *)readbuffer,BUFFERLENGTH);
    CU_ASSERT_EQUAL(nb_read,strlen(WRITECMD));
    CU_ASSERT_EQUAL(strcmp(readbuffer,WRITECMD),0);

    memset(readbuffer,0,sizeof(readbuffer));

    testatmachinedevice_Write_data();
    nb_read = atmachinedevice_Read(&uartDevice,(uint8_t *)readbuffer,BUFFERLENGTH);
    CU_ASSERT_EQUAL(nb_read,strlen(WRITEDATA));
    CU_ASSERT_EQUAL(strcmp(readbuffer,WRITEDATA),0);

    CU_PASS("atmachinedevice_Read");
}

/* The suite initialization function.
 * Opens the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{

    uint32_t fd = open(UART_PORT, O_CREAT | O_RDWR | O_SYNC, S_IRUSR | S_IWUSR);

    sprintf(uartDevice.name,"device");
    uartDevice.handle     = fd;
    uartDevice.deviceItf.read         =(le_result_t(*)(uint32_t,void*,uint32_t))      read;
    uartDevice.deviceItf.write        =(le_result_t(*)(uint32_t,const void*,uint32_t))write;
    uartDevice.deviceItf.io_control   =(le_result_t(*)(uint32_t,uint32_t,void*))      NULL;
    uartDevice.deviceItf.close        =(le_result_t(*)(uint32_t))                   close;

    return 0;
}

/* The suite cleanup function.
 * Closes the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{
    close(uartDevice.handle);
    unlink(UART_PORT);

    return 0;
}

COMPONENT_INIT
{
    int result = EXIT_SUCCESS;

    // Init the test case / test suite data structures

    CU_TestInfo test[] =
    {
        { "Test atmachinedevice_Read",   testatmachinedevice_Read },
        CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
        { "device tests",                init_suite, clean_suite, test },
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

    //CU_basic_set_mode(CU_BRM_NORMAL);
    CU_basic_set_mode(CU_BRM_VERBOSE);

    // It is possible to just run the batch tests here, using CU_basic_run_suite(), but
    // I think there is value in running all suites, even if the interactive tests are
    // not fully verified here.
    //CU_basic_run_suite(CU_get_suite("Batch tests"));
    CU_basic_run_tests();

    // Output summary of failures, if there were any
    if ( CU_get_number_of_failures() > 0 )
    {
        fprintf(stdout,"\n [START]List of Failure :\n");
        CU_basic_show_failures(CU_get_failure_list());
        fprintf(stdout,"\n [STOP]List of Failure\n");
        result = EXIT_FAILURE;
    }

    CU_cleanup_registry();
    exit(result);
}
