/**
 * Simple test of Legato semaphore API
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#include "legato.h"

#define GLOBAL_SEM_TIMEOUT 5
#define NB_THREADS         2

le_sem_Ref_t PingSemPtr = NULL;
le_sem_Ref_t PongSemPtr = NULL;

static void testCreateDelete
(
    void
)
{
    le_sem_Ref_t semPtr = NULL;
    le_sem_Ref_t semPtr2 = NULL;
    le_sem_Ref_t semPtr3 = NULL;

    LE_TEST_INFO("-------- Testing semaphore creation/deletion --------");
    semPtr = le_sem_Create("SEMAPHORE-1", 10);
    LE_TEST_ASSERT(semPtr != NULL, "Multiple tokens semaphore created.");

    semPtr2 = le_sem_Create("SEMAPHORE-2", 1);
    LE_TEST_ASSERT(semPtr2 != NULL, "Single token semaphore created.");

    semPtr3 = le_sem_Create("SEMAPHORE-3", 0);
    LE_TEST_ASSERT(semPtr3 != NULL, "Empty token semaphore created.");

    le_sem_Delete(semPtr);
    LE_TEST_OK(true, "Multiple tokens semaphore deleted.");
    le_sem_Delete(semPtr2);
    LE_TEST_OK(true, "Single token semaphore deleted.");
    le_sem_Delete(semPtr3);
    LE_TEST_OK(true, "Empty token semaphore deleted.");
}

static void testWait
(
    void
)
{
    int remainTok = 0;
    le_sem_Ref_t semPtr = NULL;

    LE_TEST_INFO("-------- Testing semaphore wait/count --------");
    semPtr = le_sem_Create("SEMAPHORE-1", 3);
    LE_TEST_ASSERT(semPtr != NULL, "3-tokens semaphore created.");

    le_sem_Wait(semPtr);
    LE_TEST_INFO("Semaphore wait called.");
    remainTok = le_sem_GetValue(semPtr);
    LE_TEST_ASSERT(remainTok == 2, "Semaphore GetValue: 2 tokens remaining.");

    le_sem_Wait(semPtr);
    LE_TEST_INFO("Semaphore wait called.");
    remainTok = le_sem_GetValue(semPtr);
    LE_TEST_ASSERT(remainTok == 1, "Semaphore GetValue: 1 token remaining.");

    le_sem_Wait(semPtr);
    LE_TEST_INFO("Semaphore wait called.");
    remainTok = le_sem_GetValue(semPtr);
    LE_TEST_ASSERT(remainTok == 0, "Semaphore GetValue: no remaining token.");

    le_sem_Delete(semPtr);
    LE_TEST_OK(true, "3-tokens semaphore deleted.");
}

void testFindSemaphore(void)
{
    LE_TEST_BEGIN_SKIP(!LE_CONFIG_IS_ENABLED(LE_CONFIG_SEM_NAMES_ENABLED), 10);
    le_sem_Ref_t semPtr=NULL,semPtr2=NULL,FindPtr=NULL;
    //Not Found semaphore should return NULL
    semPtr = le_sem_FindSemaphore("SEMAPHORE-1");
    LE_TEST_OK(semPtr == NULL, "find non-exitant semaphore fails");

    //Create sem-1
    semPtr = le_sem_Create("SEMAPHORE-1", 1);
    LE_TEST_ASSERT(semPtr != NULL, "create SEMAPHORE-1");

    //Create Sem-2
    semPtr2 = le_sem_Create("SEMAPHORE-2", 1);
    LE_TEST_ASSERT(semPtr2 != NULL, "create SEMAPHORE-2");

    //Find sem-1 and match their reference
    FindPtr = le_sem_FindSemaphore("SEMAPHORE-1");
    LE_TEST_OK(FindPtr !=  NULL, "SEMAPHORE-1 exists");
    LE_TEST_OK(semPtr == FindPtr, "found SEMAPHORE-1 matches actual SEMAPHORE-1");

    FindPtr = le_sem_FindSemaphore("SEMAPHORE-2");
    LE_TEST_OK(FindPtr !=  NULL, "SEMAPHORE-2 exists");
    LE_TEST_OK(semPtr2 == FindPtr, "found SEMAPHORE-2 matches actual SEMAPHORE-2");

    //Delete the Sem-2 and search again sem-2
    le_sem_Delete(FindPtr);
    LE_TEST_OK(true, "Destroy SEMAPHORE-2");
    FindPtr = le_sem_FindSemaphore("SEMAPHORE-2");
    LE_TEST_OK(FindPtr == NULL, "find deleted semaphore fails");

    //Delete sem-1
    le_sem_Delete(semPtr);
    LE_TEST_OK(true, "Destroy SEMAPHORE-1");
    LE_TEST_END_SKIP();
}

static void testTryWait
(
    void
)
{
    le_sem_Ref_t semPtr = NULL;
    int remainTok = 0;
    le_result_t result;

    LE_TEST_INFO("-------- Testing semaphore tryWait/count --------");
    semPtr = le_sem_Create("SEMAPHORE-1", 2);
    LE_TEST_ASSERT(semPtr != NULL, "2-tokens semaphore created.");

    result = le_sem_TryWait(semPtr);
    LE_TEST_OK(result == LE_OK, "Semaphore tryWait successfully called.");
    remainTok = le_sem_GetValue(semPtr);
    LE_TEST_OK(remainTok == 1, "Semaphore GetValue: 1 token remaining.");

    result = le_sem_TryWait(semPtr);
    LE_TEST_OK(result == LE_OK, "Semaphore tryWait successfully called.");
    remainTok = le_sem_GetValue(semPtr);
    LE_TEST_OK(remainTok == 0, "Semaphore GetValue: no remaining token.");

    result = le_sem_TryWait(semPtr);
    LE_TEST_OK(result == LE_WOULD_BLOCK, "Empty semaphore tryWait failed.");

    le_sem_Delete(semPtr);
    LE_TEST_OK(true, "2-tokens semaphore deleted.");
}

static void testPostGetValue
(
    void
)
{
    le_sem_Ref_t semPtr = NULL;
    int remainTok = 0;

    LE_TEST_INFO("-------- Testing semaphore post/count --------");
    semPtr = le_sem_Create("SEMAPHORE-1", 10);
    LE_TEST_ASSERT(semPtr != NULL, "10-tokens semaphore created.");

    le_sem_Post(semPtr);
    LE_TEST_INFO("Semaphore post called.");
    remainTok = le_sem_GetValue(semPtr);
    LE_TEST_ASSERT(remainTok == 11, "Semaphore GetValue: 11 tokens remaining.");

    le_sem_Post(semPtr);
    LE_TEST_INFO("Semaphore post called.");
    remainTok = le_sem_GetValue(semPtr);
    LE_TEST_ASSERT(remainTok == 12, "Semaphore GetValue: 12 tokens remaining.");

    le_sem_Post(semPtr);
    LE_TEST_INFO("Semaphore post called.");
    remainTok = le_sem_GetValue(semPtr);
    LE_TEST_ASSERT(remainTok == 13, "Semaphore GetValue: 13 tokens remaining.");

    le_sem_Delete(semPtr);
    LE_TEST_OK(true, "13-tokens semaphore deleted.");
}

static void* testSyncThreadFunc0
(
    void* contextPtr
)
{
    int i = 3;
    le_result_t res = LE_FAULT;
    le_clk_Time_t timeout = {GLOBAL_SEM_TIMEOUT, 0};

    LE_UNUSED(contextPtr);

    while (i--)
    {
        res = le_sem_WaitWithTimeOut(PingSemPtr, timeout);
        LE_TEST_ASSERT(res == LE_OK, "[SyncThread-0] Synchronization semaphore unlocked.");
        LE_TEST_INFO("[SyncThread-0] Semaphore wait successfully called.");
        le_sem_Post(PongSemPtr);
        LE_TEST_INFO("[SyncThread-0] Semaphore post called.");
    }
    return NULL;
}

static void* testSyncThreadFunc1
(
    void* contextPtr
)
{
    int i = 3;
    le_result_t res = LE_FAULT;
    le_clk_Time_t timeout = {GLOBAL_SEM_TIMEOUT, 0};

    LE_UNUSED(contextPtr);

    while (i--)
    {
        le_sem_Post(PingSemPtr);
        LE_TEST_INFO("[SyncThread-1] Semaphore post called.");
        res = le_sem_WaitWithTimeOut(PongSemPtr, timeout);
        LE_TEST_ASSERT(res == LE_OK, "[SyncThread-1] Synchronization semaphore unlocked.");
        LE_TEST_INFO("[SyncThread-1] Semaphore wait successfully called.");
    }
    return NULL;
}

static void testSyncThreads
(
    void
)
{
    int i;
    int remainTok = 0;
    le_thread_Ref_t thread[NB_THREADS] = {NULL};

    LE_TEST_INFO("-------- Testing semaphore synchronization --------");

    PingSemPtr = le_sem_Create("PingSemaphore", 0);
    LE_TEST_ASSERT(PingSemPtr != NULL, "0-token global ping semaphore created.");
    remainTok = le_sem_GetValue(PingSemPtr);
    LE_TEST_ASSERT(remainTok == 0, "Ping semaphore GetValue: no remaining token.");

    PongSemPtr = le_sem_Create("PongSemaphore", 0);
    LE_TEST_ASSERT(PongSemPtr != NULL, "0-token global pong semaphore created.");
    remainTok = le_sem_GetValue(PongSemPtr);
    LE_TEST_ASSERT(remainTok == 0, "Pong semaphore GetValue: no remaining token.");

    thread[0] = le_thread_Create("SyncThread-0", testSyncThreadFunc0, NULL);
    LE_TEST_ASSERT(thread[0] != NULL,"SyncThread-0 created." );
    le_thread_SetJoinable(thread[0]);
    le_thread_Start(thread[0]);
    LE_TEST_INFO("SyncThread-0 started." );

    thread[1] = le_thread_Create("SyncThread-1", testSyncThreadFunc1, NULL);
    LE_TEST_ASSERT(thread[1] != NULL,"SyncThread-1 created." );
    le_thread_SetJoinable(thread[1]);
    le_thread_Start(thread[1]);
    LE_TEST_INFO("SyncThread-1 started." );

    for (i = 0; i < NB_THREADS; i ++)
    {
        le_thread_Join(thread[i], NULL);
    }
    le_sem_Delete(PingSemPtr);
    LE_TEST_OK(true, "0-token global ping semaphore deleted.");
    le_sem_Delete(PongSemPtr);
    LE_TEST_OK(true, "0-token global pong semaphore deleted.");
}

COMPONENT_INIT
{
    LE_TEST_INFO("======== BEGIN SEMAPHORE TEST ========");

    LE_TEST_PLAN(47);

    testCreateDelete();
    testWait();
    testTryWait();
    testPostGetValue();
    testSyncThreads();
    testFindSemaphore();

    LE_TEST_INFO("======== SEMAPHORE TEST COMPLETE (PASSED) ========");
    LE_TEST_EXIT;
}
