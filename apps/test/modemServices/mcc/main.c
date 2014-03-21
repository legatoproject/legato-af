 /**
  * This module is for unit testing of the modemServices component.
  *
  *
  * Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
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
#include "le_ms.h"

//--------------------------------------------------------------------------------------------------
/**
 * Main.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* test(void* context)
{
    // Init the test case / test suite data structures
    CU_TestInfo mcctest[] =
    {
        { "Test le_mcc_Profile()",  Testle_mcc_Profile },
        { "Test le_mcc_Call()",     Testle_mcc_Call },
        CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
        { "MCC tests",                NULL, NULL, mcctest },
        CU_SUITE_INFO_NULL,
    };

#ifndef AUTOMATIC
    getTel();
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

    le_event_RunLoop();
//     CU_cleanup_registry();
//     exit(CU_get_error());
}



static void init()
{
    le_ms_Init();

    le_thread_Start(le_thread_Create("MCCTest",test,NULL));
}

LE_EVENT_INIT_HANDLER
{
    init();
}
