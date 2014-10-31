 /**
  * This module is for unit testing of the modemServices component.
  *
  *
  * Copyright (C) Sierra Wireless, Inc. 2013-2014. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <semaphore.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>

#include "main.h"

//--------------------------------------------------------------------------------------------------
/**
 * Main.
 *
 */
//--------------------------------------------------------------------------------------------------
static void test(void* context)
{
    // Init the test case / test suite data structures

    CU_TestInfo smstest[] =
    {
        { "Test le_sms_SetGetSmsCenterAddress()", Testle_sms_SetGetSmsCenterAddress },
        { "Test le_sms_SetGetText()",    Testle_sms_SetGetText },
        { "Test le_sms_SetGetBinary()",  Testle_sms_SetGetBinary },
        { "Test le_sms_SetGetPDU()",     Testle_sms_SetGetPDU },
        { "Test le_sms_ReceivedList()",  Testle_sms_ReceivedList },
        { "Test le_sms_SendBinary()",    Testle_sms_SendBinary },
        { "Test le_sms_SendText()",      Testle_sms_SendText },
#if 0
        { "Test le_sms_SendPdu()",       Testle_sms_SendPdu },
#endif
        CU_TEST_INFO_NULL,
    };


    CU_SuiteInfo suites[] =
    {
        { "SMS tests",                NULL, NULL, smstest },
        CU_SUITE_INFO_NULL,
    };

    fprintf(stderr, "Please ensure that there is enough space on SIM to receive new SMS messages!\n");

#ifndef AUTOMATIC
    GetTel();
#endif

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

}

COMPONENT_INIT
{
    test(NULL);
}
