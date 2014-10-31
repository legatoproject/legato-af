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
#include <interfaces.h>

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
static void* test(void* context)
{

    // Init the test case / test suite data structures
    CU_TestInfo mrctest[] =
    {
        { "Test le_mrc_GetStateAndQual()",           Testle_mrc_GetStateAndQual },
        { "Test le_mrc_GetRadioAccessTechInUse()",   Testle_mrc_GetRat },
        { "Test le_mrc_GetNeighboringCellsInfo()",   Testle_mrc_GetNeighboringCellsInfo },
        { "Test le_mrc_NetRegHdlr()",                Testle_mrc_NetRegHdlr },
        { "Test le_mrc_RatHdlr()",                   Testle_mrc_RatHdlr },
#if 0
        { "Test le_mrc_Power()",             Testle_mrc_Power },
#endif
         CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
        { "MRC tests",                NULL, NULL, mrctest },
        CU_SUITE_INFO_NULL,
    };

    le_mrc_ConnectService();

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
}

COMPONENT_INIT
{
    le_thread_Start(le_thread_Create("MRCTest",test,NULL));
}
