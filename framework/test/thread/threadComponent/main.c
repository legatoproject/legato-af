//--------------------------------------------------------------------------------------------------
/**
 * This file contains the main code that kicks off all the tests and runs the final test wrap-up.
 *
 * When all the tests have finished running, a memory pool object's reference count will reach
 * zero and it's destructor will run a "check completion status" function for each test.  If all
 * of those return true, then the process will exit with EXIT_SUCCESS.  Otherwise, the process
 * will exit with EXIT_FAILURE (or will hang forever, if the destructor never runs or one of
 * the completion status check functions deadlocks).
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"

#include "forkJoinMutex.h"
#include "externalThreadApi.h"
#include "priority.h"
#include "staticThread.h"

const char TestNameStr[] = "Thread Test";


// -------------------------------------------------------------------------------------------------
/**
 * Collect test results and exit.
 */
// -------------------------------------------------------------------------------------------------
static void FinishTest
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    LE_TEST_INFO("All tests have signalled completion.  Thread '%s' is checking results...",
                 le_thread_GetMyName());

    fjm_CheckResults();
    eta_CheckResults();
    prio_CheckResults();
    static_CheckResults();

    LE_TEST_EXIT;
}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    // Run before fjm test, as fjm test may leave a few threads running
    // after test is "complete".
    eta_Start();
    fjm_Start();
    prio_Start();
    static_Start();

    FinishTest();
}
