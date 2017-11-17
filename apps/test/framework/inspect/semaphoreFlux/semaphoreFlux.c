/*
 * This app creates and deletes semaphores...
 *
 */

#include "legato.h"
#include "limit.h"
#include "thread.h"


// Arguments
static const char* TestType;
static long DelInv;
static long ThreadNum;

static le_thread_Ref_t* ThreadRefArray; // array storing references to the threads

// data type associating a semaphore that a thread waits on, and the thread ref.
typedef struct
{
    le_thread_Ref_t threadRef;
    le_sem_Ref_t semRef;
}
SemRef_t;

//static le_sem_Ref_t* SemRefArray; // array storing references to the semaphores.
static SemRef_t* SemRefArray; // array storing references to the semaphores.
static long SemCreateIdx = 0; // the index of the SemRefArray at which the last sempahore creation has ended at.

le_mutex_Ref_t SemIndexMutexRef; // for accessing "SemCreateIdx"

le_sem_Ref_t SemaRef;



// A thread function that attempts to wait on the semaphore passed in as context.
static void* WaitOnSem
(
    void* context
)
{
    le_sem_Ref_t sem = (le_sem_Ref_t)context;
    le_sem_Wait(sem);
    le_event_RunLoop();
    return NULL;
}


// This is testing if Semaphore_t.waitingList is displayed correctly.
// Thread 1, 2, and 3 are all waiting on Sem1. Thread4 is waiting on Sem2. Thread5 is waiting on Sem3.
// Therefore the expected result is that Sem1's waiting list has Thread 1, 2, and 3.
// Sem2's waiting list has Thread4.  Sem3's waiting list has Thread5.
void testWaitingList
(
    void
)
{
    le_sem_Ref_t sema1Ref = le_sem_Create("Semaphore1", 0);
    le_sem_Ref_t sema2Ref = le_sem_Create("Semaphore2", 0);
    le_sem_Ref_t sema3Ref = le_sem_Create("Semaphore3", 0);

    // create thread refs
    le_thread_Ref_t thread1Ref = le_thread_Create("Thread1", WaitOnSem, (void*)sema1Ref);
    le_thread_Ref_t thread2Ref = le_thread_Create("Thread2", WaitOnSem, (void*)sema1Ref);
    le_thread_Ref_t thread3Ref = le_thread_Create("Thread3", WaitOnSem, (void*)sema1Ref);
    le_thread_Ref_t thread4Ref = le_thread_Create("Thread4", WaitOnSem, (void*)sema2Ref);
    le_thread_Ref_t thread5Ref = le_thread_Create("Thread5", WaitOnSem, (void*)sema3Ref);

    // start the threads
    le_thread_Start(thread1Ref);
    le_thread_Start(thread2Ref);
    le_thread_Start(thread3Ref);
    le_thread_Start(thread4Ref);
    le_thread_Start(thread5Ref);
}


/////////////////////////////////////////////
/*
 * Functions relevant to waiting and posting semas
 */
/////////////////////////////////////////////

// A thread "main" function which creates a series of sems.
static void* ThreadCreateSem
(
    void* context
)
{
    char semNameBuffer[LIMIT_MAX_SEMAPHORE_NAME_BYTES] = {0};


    LE_INFO("Thread [%s] has started. Waiting on a semaphore.", le_thread_GetMyName());

    le_mutex_Lock(SemIndexMutexRef);

    snprintf(semNameBuffer, LIMIT_MAX_SEMAPHORE_NAME_BYTES, "[%s]Sem%ld",
             le_thread_GetMyName(), SemCreateIdx);

    le_sem_Ref_t sem = le_sem_Create(semNameBuffer, 0);

    SemRefArray[SemCreateIdx] = (SemRef_t){le_thread_GetCurrent(), sem};

    SemCreateIdx++;

    le_mutex_Unlock(SemIndexMutexRef);

    LE_INFO("In thread [%s], about to wait sem", le_thread_GetMyName());

    // notify the calling thread that this thread is about to wait on its sema.
    le_sem_Post(SemaRef);

    le_sem_Wait(sem);
    LE_INFO("In thread [%s], sema is posted", le_thread_GetMyName());

    le_event_RunLoop();
    return NULL;
}


// Create all semaphores for the specified number of threads. Since there's no semaphore list,
// one semaphore is created per thread.
void createAllSemaphores
(
    void
)
{
    char threadNameBuffer[MAX_THREAD_NAME_SIZE] = {0};

    long threadCnt = 0;
    while (threadCnt < ThreadNum)
    {
        snprintf(threadNameBuffer, MAX_THREAD_NAME_SIZE, "Thread%ld", threadCnt);

        // Store the thread references in an array
        ThreadRefArray[threadCnt] = le_thread_Create(threadNameBuffer, ThreadCreateSem, NULL);

        le_thread_Start(ThreadRefArray[threadCnt]);

        threadCnt++;
    }

    LE_INFO("========== Created all threads ===========");

    // waiting for all threads to start waiting on their sema.
    long cnt = 0;
    while (cnt < ThreadNum)
    {
        le_sem_Wait(SemaRef);
        cnt++;
    }

    LE_INFO("========== All threads have started waiting on their semaphores ===========");
}


static void PostSemInThread
(
    int threadIdx
)
{
    if (threadIdx >= ThreadNum)
    {
        LE_ERROR("thread index out of range.");
        return;
    }

    int i;
    for (i = 0; i < ThreadNum; i++)
    {
        if (SemRefArray[i].threadRef == ThreadRefArray[threadIdx])
        {
            char threadNameBuffer[MAX_THREAD_NAME_SIZE];
            le_thread_GetName(SemRefArray[i].threadRef, threadNameBuffer, MAX_THREAD_NAME_SIZE);
            LE_INFO("About to post the semaphore being waited in thread [%s]", threadNameBuffer);
            le_sem_Post(SemRefArray[i].semRef);
            break;
        }
    }

    if (i == ThreadNum)
    {
        LE_ERROR("Failed to post semaphore for the %dth thread.", threadIdx);
    }
}


/////////////////////////////////////////////
/*
 * Argument management and other house-keeping
 */
/////////////////////////////////////////////
static void PrintHelp
(
    void
)
{
    LE_ERROR("Usage: semaphoreFlux TestWaitingList");
    LE_ERROR("       semaphoreFlux [1toN-1Threads | Sem1toN-1Threads] [delete interval] [number of threads]");
    LE_ERROR("       semaphoreFlux [Sem1stThread | SemMidThread | 1stThread | MidThread | None] [number of threads]");
    LE_ERROR(" ");
    LE_ERROR("       [TestWaitingList] create a scenario to display the waiting list");
    LE_ERROR(" ");
    LE_ERROR("       The following options create N threads, each of which waits on a sema, and then...");
    LE_ERROR("       [1toN-1Threads] cancels threads from 1st to N-1th");
    LE_ERROR("       [Sem1toN-1Threads] posts to the sema being waited on in threads 1 to N-1");
    LE_ERROR("       [Sem1stThread] posts to the sema in the 1st thread");
    LE_ERROR("       [SemMidThread] posts to the sema in the mid thread");
    LE_ERROR("       [1stThread] cancels the 1st thread");
    LE_ERROR("       [MidThread] cancels the mid thread");
    LE_ERROR("       [None] doesn't cancel threads or post to semas");
    LE_ERROR(" ");
    LE_ERROR("       [delete interval] is in micro-secs");

    exit(EXIT_FAILURE);
}

static void DelInvArgHandler
(
    const char* arg
)
{
    DelInv = strtol(arg, NULL, 0);
}

static void NumThreadArgHandler
(
    const char* arg
)
{
    ThreadNum = strtol(arg, NULL, 0);
}

static void TestTypeArgHandler
(
    const char* arg
)
{
    TestType = arg;

    if (strcmp(arg, "TestWaitingList") == 0)
    {
        // do nothing
    }
    else if ((strcmp(arg, "1toN-1Threads") == 0) ||
             (strcmp(arg, "Sem1toN-1Threads") == 0))
    {
        le_arg_AddPositionalCallback(DelInvArgHandler);
        le_arg_AddPositionalCallback(NumThreadArgHandler);
    }
    else if ((strcmp(arg, "Sem1stThread") == 0) ||
             (strcmp(arg, "SemMidThread") == 0) ||
             (strcmp(arg, "1stThread") == 0) ||
             (strcmp(arg, "MidThread") == 0) ||
             (strcmp(arg, "None") == 0))
    {
        le_arg_AddPositionalCallback(NumThreadArgHandler);
    }
    else
    {
        PrintHelp();
    }
}

static void Init
(
    void
)
{
    // mutex for accessing the sem index variable.
    SemIndexMutexRef = le_mutex_CreateNonRecursive("SemIndexMutex");

    // syncrhonizing among threads for waiting/posting semas.
    SemaRef = le_sem_Create("semaphoreFluxSemaphore", 0);

    // Initializing the array storing thread refs.
    ThreadRefArray = malloc(ThreadNum * sizeof(le_thread_Ref_t));

    // Initializing the array storing sem refs.
    SemRefArray = malloc(ThreadNum * sizeof(SemRef_t));
}

static void RunTests
(
    void
)
{
    if (strcmp(TestType, "TestWaitingList") == 0)
    {
        testWaitingList();
    }
    else if (strcmp(TestType, "1toN-1Threads") == 0)
    {
        createAllSemaphores();

        int i = 0;
        while (i <= (ThreadNum - 2))
        {
            LE_INFO("Cancelling the %dth thread", i);
            le_thread_Cancel(ThreadRefArray[i]);
            usleep(DelInv);
            i++;
        }
    }
    else if (strcmp(TestType, "Sem1toN-1Threads") == 0)
    {
        createAllSemaphores();

        int i = 0;
        while (i <= (ThreadNum - 2))
        {
            PostSemInThread(i);
            usleep(DelInv);
            i++;
        }
    }
    else if (strcmp(TestType, "Sem1stThread") == 0)
    {
        createAllSemaphores();
        PostSemInThread(0);
    }
    else if (strcmp(TestType, "SemMidThread") == 0)
    {
        createAllSemaphores();
        PostSemInThread(ThreadNum / 2);
    }
    else if (strcmp(TestType, "1stThread") == 0)
    {
        createAllSemaphores();
        LE_INFO("Cancelling the 1st thread");
        le_thread_Cancel(ThreadRefArray[0]);
    }
    else if (strcmp(TestType, "MidThread") == 0)
    {
        createAllSemaphores();
        LE_INFO("Cancelling the middle thread");
        le_thread_Cancel(ThreadRefArray[ThreadNum / 2]);
    }
    else if (strcmp(TestType, "None") == 0)
    {
        createAllSemaphores();
    }
    else
    {
        // should never get here.
        PrintHelp();
    }
}


COMPONENT_INIT
{
    le_arg_AddPositionalCallback(TestTypeArgHandler);
    le_arg_Scan();

    Init();

    RunTests();

    LE_INFO("================== END of semaphoreFlux =====================");
}
