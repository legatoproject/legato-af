#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <limits.h>

#include "legato.h"

#include "CUnit/Console.h"
#include "CUnit/Basic.h"

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

void testCreateDestroy(void)
{
    le_sem_Ref_t semPtr=NULL,semPtr2=NULL;

    semPtr     = le_sem_Create( "SEMAPHORE-1", 10);
    CU_ASSERT_PTR_NOT_EQUAL(semPtr, NULL);

    semPtr2    = le_sem_Create( "SEMAPHORE-2", 1);
    CU_ASSERT_PTR_NOT_EQUAL(semPtr2, NULL);


    le_sem_Delete(semPtr);
    CU_PASS("Destruct semaphore\n");
    le_sem_Delete(semPtr2);
    CU_PASS("Destruct semaphore\n");
}

void testWait(void)
{
    le_sem_Ref_t semPtr=NULL;

    semPtr     = le_sem_Create( "SEMAPHORE-1", 3);
    CU_ASSERT_PTR_NOT_EQUAL(semPtr, NULL);

    le_sem_Wait(semPtr);
    CU_ASSERT_EQUAL(le_sem_GetValue(semPtr),2);
    le_sem_Wait(semPtr);
    CU_ASSERT_EQUAL(le_sem_GetValue(semPtr),1);
    le_sem_Wait(semPtr);
    CU_ASSERT_EQUAL(le_sem_GetValue(semPtr),0);

    le_sem_Delete(semPtr);
    CU_PASS("Destruct semaphore\n");
}

void testFindSemaphore(void)
{
    le_sem_Ref_t semPtr=NULL,semPtr2=NULL,FindPtr=NULL;
    //Not Found semaphore should return NULL
    semPtr = le_sem_FindSemaphore("SEMAPHORE-1");
    CU_ASSERT_EQUAL(semPtr, NULL);

    //Create sem-1
    semPtr = le_sem_Create("SEMAPHORE-1", 1);
    CU_ASSERT_PTR_NOT_EQUAL(semPtr, NULL);

    //Create Sem-2
    semPtr2 = le_sem_Create("SEMAPHORE-2", 1);
    CU_ASSERT_PTR_NOT_EQUAL(semPtr2, NULL);

    //Find sem-1 and match their reference
    FindPtr = le_sem_FindSemaphore("SEMAPHORE-1");
    CU_ASSERT_PTR_NOT_EQUAL(FindPtr, NULL);
    CU_ASSERT_EQUAL(semPtr, FindPtr);

    FindPtr = le_sem_FindSemaphore("SEMAPHORE-2");
    CU_ASSERT_PTR_NOT_EQUAL(FindPtr, NULL);
    CU_ASSERT_EQUAL(semPtr2, FindPtr);

    //Delete the Sem-2 and search again sem-2
    le_sem_Delete(FindPtr);
    CU_PASS("Destruct semaphore\n");
    FindPtr = le_sem_FindSemaphore("SEMAPHORE-2");
    CU_ASSERT_EQUAL(FindPtr, NULL);

    //Delete sem-1
    le_sem_Delete(semPtr);
    CU_PASS("Destruct semaphore\n");
}

void testTryWait(void)
{
    le_sem_Ref_t semPtr=NULL;
    le_result_t     result;

    semPtr     = le_sem_Create( "SEMAPHORE-1", 2);
    CU_ASSERT_PTR_NOT_EQUAL(semPtr, NULL);

    result=le_sem_TryWait(semPtr);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(le_sem_GetValue(semPtr),1);
    result=le_sem_TryWait(semPtr);
    CU_ASSERT_EQUAL(result,LE_OK);
    CU_ASSERT_EQUAL(le_sem_GetValue(semPtr),0);
    result=le_sem_TryWait(semPtr);
    CU_ASSERT_EQUAL(result,LE_WOULD_BLOCK);

    le_sem_Delete(semPtr);
    CU_PASS("Destruct semaphore\n");
}

void testPostOK(void)
{
    le_sem_Ref_t semPtr=NULL;

    semPtr     = le_sem_Create( "SEMAPHORE-1", 10);
    CU_ASSERT_PTR_NOT_EQUAL(semPtr, NULL);

    le_sem_Post(semPtr);
    CU_ASSERT_EQUAL(le_sem_GetValue(semPtr),11);

    le_sem_Delete(semPtr);
    CU_PASS("Destruct semaphore\n");
}

void testGetValue(void)
{
    le_sem_Ref_t semPtr=NULL;

    semPtr     = le_sem_Create( "SEMAPHORE-1", 10);
    CU_ASSERT_PTR_NOT_EQUAL(semPtr, NULL);

    le_sem_Post(semPtr);
    CU_ASSERT_EQUAL(le_sem_GetValue(semPtr),11);
    le_sem_Post(semPtr);
    CU_ASSERT_EQUAL(le_sem_GetValue(semPtr),12);
    le_sem_Post(semPtr);
    CU_ASSERT_EQUAL(le_sem_GetValue(semPtr),13);

    le_sem_Delete(semPtr);
    CU_PASS("Destruct semaphore\n");
}


// void testPostKO(void)
// {
//     le_sem_Ref_t semPtr=NULL;
//
//     semPtr     = le_sem_Create( "SEMAPHORE-1", SEM_VALUE_MAX-2);
//     CU_ASSERT_PTR_NOT_EQUAL(semPtr, NULL);
//
//     le_sem_Post(semPtr);
//     CU_PASS("le_sem_Post semaphore\n");
//     CU_ASSERT_EQUAL(le_sem_GetValue(semPtr),SEM_VALUE_MAX-1);
//     le_sem_Post(semPtr);
//     CU_PASS("le_sem_Post semaphore\n");
//     CU_ASSERT_EQUAL(le_sem_GetValue(semPtr),SEM_VALUE_MAX);
//     le_sem_Post(semPtr); // program should exit
//     CU_FAIL("failed passed through post");
//
//     le_sem_Delete(semPtr);
//     CU_PASS("Destruct semaphore\n");
// }
//
// void testPtrNullCreate()
// {
//     le_sem_Ref_t semPtr = le_sem_Create( "", 10);     // program should exit
//     CU_ASSERT_PTR_EQUAL(semPtr, NULL);
// }
//
// void testPtrNullDestroy()
// {
//     le_sem_Delete(NULL);     // program should exit
//     CU_FAIL("Destroy succeed, not possible");
// }
//
// void testPtrNullFind()
// {
//     le_sem_FindSemaphore(NULL);     // program should exit
//     CU_FAIL("Find succeed, not possible");
// }

// void testPtrNullWait()
// {
//     le_sem_Wait(NULL);     // program should exit
//     CU_FAIL("Wait succeed, not possible");
// }
//
// void testPtrNullTryWait()
// {
//     le_sem_TryWait(NULL);     // program should exit
//     CU_FAIL("TryWait succeed, not possible");
// }
//
// void testPtrNullPost()
// {
//     le_sem_Post(NULL);     // program should exit
//     CU_FAIL("Post succeed, not possible");
// }

#define NB_THREADS 15
#define SEM_NAME_1    "SEMAPHORE-1"
#define SEM_NAME_2    "SEMAPHORE-2"
__thread    int cpt;
le_sem_Ref_t GSemPtr=NULL;
le_sem_Ref_t GSem2Ptr=NULL;
void * fonction_thread ()
{
    cpt=100;

       while (cpt--) {
            le_sem_Wait(GSemPtr);
            fprintf(stdout, "\n%d : thread '%s' has %s %d\n",
                    cpt,le_thread_GetMyName(),SEM_NAME_1,le_sem_GetValue(GSemPtr));
            CU_PASS("thread GSemPtr get");
            le_sem_Wait(GSem2Ptr);
            fprintf(stdout, "\n%d : thread '%s' has %s %d\n",
                    cpt,le_thread_GetMyName(),SEM_NAME_2,le_sem_GetValue(GSem2Ptr));
            CU_PASS("thread GSem2Ptr get");
            usleep(10000);
            le_sem_Post(GSem2Ptr);
            fprintf(stdout, "\n%d : thread '%s' release %s %d\n",
                    cpt,le_thread_GetMyName(),SEM_NAME_2,le_sem_GetValue(GSem2Ptr));
            CU_PASS("thread GSemPtr2 UnLocked");
            le_sem_Post(GSemPtr);
            fprintf(stdout, "\n%d : thread '%s' release %s %d\n",
                    cpt,le_thread_GetMyName(),SEM_NAME_1,le_sem_GetValue(GSemPtr));
            CU_PASS("thread GSemPtr UnLocked");
    }

    return NULL;
}

void launch_thread()
{
        int i;
        le_thread_Ref_t thread[NB_THREADS];

        GSemPtr  = le_sem_Create( SEM_NAME_1, 5);
        GSem2Ptr = le_sem_Create( SEM_NAME_2, 2);

        CU_ASSERT_PTR_NOT_EQUAL(GSemPtr, NULL);
        for (i = 0; i < NB_THREADS; i ++) {
            char threadName[20];
            snprintf(threadName,20,"Thread_%d",i);
            thread[i] = le_thread_Create(threadName, fonction_thread, NULL);
            le_thread_SetJoinable(thread[i]);
            le_thread_Start(thread[i]);
            usleep(10000);
        }
        for (i = 0; i < NB_THREADS; i ++) {
            le_thread_Join(thread[i], NULL);
        }
        le_sem_Delete(GSem2Ptr);
        le_sem_Delete(GSemPtr);
        CU_PASS("GlobalSemaphore destroy");
}

// void * fonction_process ()
// {
//     cpt=100;
//     le_sem_Ref_t Sem2Ptr=NULL;
//
//     le_sem_Ref_t SemPtr     = le_sem_Create( SEM_NAME_1, 10);
//
//     usleep(500);
//
//        while (cpt--) {
//         if ( cpt==80) {
//             Sem2Ptr    = le_sem_Create( SEM_NAME_2, 10);
//         }
//         le_sem_Wait(SemPtr);
//         fprintf(stdout, "\n%d : thread '%s' has %s %d\n",
//                 cpt,le_thread_GetMyName(),SEM_NAME_1,le_sem_GetValue(SemPtr));
//         CU_PASS("thread SemPtr get");
//         if (( cpt<=80 ) && (cpt>20)) {
//             le_sem_Wait(Sem2Ptr);
//             fprintf(stdout, "\n%d : thread '%s' has %s %d\n",
//                     cpt,le_thread_GetMyName(),SEM_NAME_2,le_sem_GetValue(Sem2Ptr));
//             CU_PASS("thread Sem2Ptr get");
//         }
//         usleep(10000);
//         if (( cpt<=80 ) && (cpt>20)) {
//             le_sem_Post(Sem2Ptr);
//             fprintf(stdout, "\n%d : thread '%s' release %s %d\n",
//                     cpt,le_thread_GetMyName(),SEM_NAME_2,le_sem_GetValue(Sem2Ptr));
//             CU_PASS("thread SemPtr2 UnLocked");
//         }
//         le_sem_Post(SemPtr);
//         fprintf(stdout, "\n%d : thread '%s' release %s %d\n",
//                 cpt,le_thread_GetMyName(),SEM_NAME_1,le_sem_GetValue(SemPtr));
//         CU_PASS("thread SemPtr UnLocked");
//         if ( cpt == 20 ) {
//             le_sem_Delete(Sem2Ptr);
//         }
//     }
//
//     le_sem_Delete(SemPtr);
//
//     return NULL;
// }
//
// void launch_Process()
// {
//         int i;
//         le_thread_Ref_t thread[NB_THREADS];
//
//         for (i = 0; i < NB_THREADS; i ++) {
//             char threadName[20];
//             snprintf(threadName,20,"Thread_%d",i);
//             thread[i] = le_thread_Create(threadName, fonction_process);
//             le_thread_SetJoinable(thread[i]);
//             le_thread_Start(thread[i]);
//         }
//         sleep(1);
//         for (i = 0; i < NB_THREADS; i ++)
//             le_thread_Join(thread[i], NULL);
// }

void testScenario1(void)
{
    launch_thread(); // thread shared
}

// void testScenario2(void)
// {
//     launch_Process(); // process shared
// }

COMPONENT_INIT
{
    CU_TestInfo test_array1[] = {
    { "create-destroy"          , testCreateDestroy },
    { "wait"                    , testWait },
    { "trywait"                 , testTryWait },
    { "post"                    , testPostOK },
    { "value"                   , testGetValue },
    { "find"                    , testFindSemaphore},
    CU_TEST_INFO_NULL,
    };

    CU_TestInfo test_array2[] = {
//     { "postko"          , testPostKO },
// //     { "NullCreateko"    , testPtrNullCreate },
//     { "NullDestroyko"   , testPtrNullDestroy },
// //     { "NullFindko"      , testPtrNullFind },
//     { "NullWaitko"      , testPtrNullWait },
//     { "NullTryWaitko"   , testPtrNullTryWait },
//     { "NullWPostko"     , testPtrNullPost },
    CU_TEST_INFO_NULL,
    };

    CU_TestInfo test_array3[] = {
    { "scenario 1: wait thread"         , testScenario1 },
//     { "scenario 2: wait process"        , testScenario2 },
    CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] = {
    { "Suite test always ok"                , init_suite, clean_suite, test_array1 },
    { "Suite test that should EXIT_FAILURE" , init_suite, clean_suite, test_array2 },
    { "Suite test with scenario"            , init_suite, clean_suite, test_array3 },
    CU_SUITE_INFO_NULL,
    };

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        exit(CU_get_error());
//         return CU_get_error();

    if ( CUE_SUCCESS != CU_register_suites(suites))
    {
        CU_cleanup_registry();
        exit(CU_get_error());
//         return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    // Output summary of failures, if there were any
    if ( CU_get_number_of_failures() > 0 )
    {
        fprintf(stdout,"\n [START]List of Failure :\n");
        CU_basic_show_failures(CU_get_failure_list());
        fprintf(stdout,"\n [STOP]List of Failure\n");
    }

    CU_cleanup_registry();
    exit(CU_get_error());
//     return CU_get_error();
}
