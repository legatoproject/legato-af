/**
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>

#include "atMachineUnsolicited.h"

/* The suite initialization function.
 * Opens the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
    atmachineunsolicited_Init();
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

// Test this function :
// atUnsolicited_t* atmachineunsolicited_Create(void);
void testatmachineunsolicited_Create()
{
    uint32_t i;
    atUnsolicited_t* newPtr = atmachineunsolicited_Create();

    CU_ASSERT_PTR_NOT_NULL(newPtr);
    for(i=0;i<sizeof(newPtr->unsolRsp);i++) {
        CU_ASSERT_EQUAL(newPtr->unsolRsp[i],0);
    }
    CU_ASSERT_PTR_NULL(newPtr->unsolicitedReportId);
    CU_ASSERT_EQUAL(newPtr->waitForExtraData,false);
    CU_ASSERT_EQUAL(newPtr->withExtraData,false);
    CU_ASSERT_PTR_NOT_NULL(&(newPtr->link));

    le_mem_Release(newPtr);
    CU_PASS("atmachineunsolicited_Create");
}

COMPONENT_INIT
{
    int result = EXIT_SUCCESS;

    // Init the test case / test suite data structures

    CU_TestInfo test[] =
    {
        { "Test atmachineunsolicited_Create", testatmachineunsolicited_Create },
        CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
        { "atunsolicited tests",      init_suite, clean_suite, test },
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
        result = EXIT_FAILURE;
    }

    CU_cleanup_registry();
    exit(result);
}
