// -------------------------------------------------------------------------------------------------
// This file contains the main code that kicks off all the tests and runs the final test wrap-up.
//
// When all the tests have finished running, a memory pool object's reference count will reach
// zero and it's destructor will run a "check completion status" function for each test.  If all
// of those return true, then the process will exit with EXIT_SUCCESS.  Otherwise, the process
// will exit with EXIT_FAILURE (or will hang forever, if the destructor never runs or one of
// the completion status check functions deadlocks).
//
// Copyright (C) Sierra Wireless Inc.
// -------------------------------------------------------------------------------------------------

#include "legato.h"

#include "forkJoinMutex.h"
#include "externalThreadApi.h"
#include "priority.h"

const char TestNameStr[] = "Thread Test";


// -------------------------------------------------------------------------------------------------
// WARNING: There's no telling what thread will run this function!
// -------------------------------------------------------------------------------------------------
static void FinishTest
(
    void* objPtr
)
// -------------------------------------------------------------------------------------------------
{
    const char** stringPtrPtr = objPtr;
    LE_INFO("objPtr = %p.", objPtr);
    LE_INFO("*stringPtrPtr = %p.", *stringPtrPtr);
    LE_ASSERT(*stringPtrPtr == TestNameStr);

    LE_INFO("All tests have signalled completion.  Thread '%s' is checking results...",
            le_thread_GetMyName());

    fjm_CheckResults();
    eta_CheckResults();
    prio_CheckResults();

    LE_INFO("======== MULTI-THREADING TESTS PASSED ========");
    exit(EXIT_SUCCESS);
}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("======== BEGIN MULTI-THREADING TESTS ========");

    le_mem_PoolRef_t poolRef = le_mem_CreatePool(TestNameStr, sizeof(char*));
    le_mem_ExpandPool(poolRef, 1);
    le_mem_SetDestructor(poolRef, FinishTest);

    const char** objPtr = le_mem_ForceAlloc(poolRef);
    *objPtr = TestNameStr;
    LE_INFO("objPtr = %p.", objPtr);
    LE_INFO("*stringPtrPtr = %p.", *objPtr);

    fjm_Start(objPtr);

    eta_Start(objPtr);

    prio_Start(objPtr);

    le_mem_Release(objPtr);
}
