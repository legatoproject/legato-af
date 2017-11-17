/*
 * This app creates and deletes mutexes...
 *
 */

// TODO: use usleep instead of nanosleep for delete interval

#include "legato.h"
#include "limit.h"
#include "thread.h"
#include "mutex.h" // just for MAX_NAME_BYTES. TODO: move MAX_NAME_BYTES to limit.h


// Arguments
static const char* TestType;
static long DelInv;
static long MutexNum;
static long ThreadNum;

static le_thread_Ref_t* ThreadRefArray; // array storing references to the threads

static long MutexCreateIdx = 0; // A global index for all mutexes for all threads.

le_mutex_Ref_t MutexIndexMutexRef; // for accessing "MutexCreateIdx"

le_sem_Ref_t SemaRef;

/*
 * Thread-specific data key, used to store mutex refs. Note that a Legato thread already stores
 * mutex refs for each thread, but there's no public API to access them.
 */
static pthread_key_t TsdMutexRefKey;

// Data type for an array storing mutex refs.
typedef struct MutexRefArray
{
    long size; // array size
    le_mutex_Ref_t* mutexRefArray;
}
MutexRefArray_t;



// A thread function that attempts to lock all the mutexes in the mutex array passed in as context.
static void* LockMutex
(
    void* context
)
{
    MutexRefArray_t* mra = (MutexRefArray_t*)context;

    int i;
    for (i = 0; i < mra->size; i++)
    {
        le_mutex_Lock(mra->mutexRefArray[i]);
    }

    le_sem_Post(SemaRef);

    le_event_RunLoop();
    return NULL;
}


// This is testing if Mutex_t.waitingList is displayed correctly.
// Thread1 successfully locks mutexes 1, 2, and 3, and then Thread 2 and 3 tries to lock mutex 1,
// and Thread 4 and 5 tries to lock mutex 3.
// Therefore the expected result is that Mutex1's waiting list has Thread2 and 3, Mutex2's waiting
// list is empty, and Mutex3's waiting list has Thread4 and 5.
void testWaitingList
(
    void
)
{
    le_mutex_Ref_t mutex1Ref = le_mutex_CreateNonRecursive("Mutex1");
    le_mutex_Ref_t mutex2Ref = le_mutex_CreateNonRecursive("Mutex2");
    le_mutex_Ref_t mutex3Ref = le_mutex_CreateNonRecursive("Mutex3");

    // create mutex arrays to be passed to each thread.
    le_mutex_Ref_t mutexRefArray1[3] = {mutex1Ref, mutex2Ref, mutex3Ref};
    le_mutex_Ref_t mutexRefArray2[1] = {mutex1Ref};
    le_mutex_Ref_t mutexRefArray3[1] = {mutex3Ref};

    // put the arrays in a data struct containing size
    MutexRefArray_t mra1 = {NUM_ARRAY_MEMBERS(mutexRefArray1), mutexRefArray1};
    MutexRefArray_t mra2 = {NUM_ARRAY_MEMBERS(mutexRefArray2), mutexRefArray2};
    MutexRefArray_t mra3 = {NUM_ARRAY_MEMBERS(mutexRefArray3), mutexRefArray3};

    // create thread refs
    le_thread_Ref_t thread1Ref = le_thread_Create("Thread1", LockMutex, (void*)&mra1);
    le_thread_Ref_t thread2Ref = le_thread_Create("Thread2", LockMutex, (void*)&mra2);
    le_thread_Ref_t thread3Ref = le_thread_Create("Thread3", LockMutex, (void*)&mra2);
    le_thread_Ref_t thread4Ref = le_thread_Create("Thread4", LockMutex, (void*)&mra3);
    le_thread_Ref_t thread5Ref = le_thread_Create("Thread5", LockMutex, (void*)&mra3);

    // start the threads
    le_thread_Start(thread1Ref);
    // Do not proceed untiil Thread1 has gotten all the mutex locks
    le_sem_Wait(SemaRef);
    le_thread_Start(thread2Ref);
    le_thread_Start(thread3Ref);
    le_thread_Start(thread4Ref);
    le_thread_Start(thread5Ref);

    // Threads 2, 3, 4, and 5 are mutex-locked and therefore can't get to Post. The function needs
    // to hang around for a bit for the mutex refs to be available for the threads.
    le_sem_Wait(SemaRef);

    LE_INFO("++++++++++++++++++  END OF testWaitingList (shouldn't get here) +++++++++++++++++++");
}


// This is testing Recursive Mutex and lock count.
// Thread1 recursively locks the same mutex 3 times.
// Therefore the expected result is that the lock count should be 3 and "recursive"
// should be true (1).
void testRecursive
(
    void
)
{
    le_mutex_Ref_t mutex1Ref = le_mutex_CreateRecursive("RecursiveMutex1");

    le_mutex_Ref_t mutexRefArray1[3] = {mutex1Ref, mutex1Ref, mutex1Ref};

    MutexRefArray_t mra1 = {NUM_ARRAY_MEMBERS(mutexRefArray1), mutexRefArray1};

    le_thread_Ref_t thread1Ref = le_thread_Create("Thread1", LockMutex, (void*)&mra1);

    le_thread_Start(thread1Ref);

    le_sem_Wait(SemaRef);

    // Keep the function around so that mutex refs are available.
    le_sem_Wait(SemaRef);

    LE_INFO("++++++++++++++++++  END OF testRecursive (shouldn't get here) +++++++++++++++++++++");
}


/////////////////////////////////////////////
/*
 * Functions relevant to create and delete mutexes
 */
/////////////////////////////////////////////

// A thread "main" function which creates a series of mutexes.
static void* ThreadCreateMutex
(
    void* context
)
{
    long mutexCount = (long)context;
    char mutexNameBuffer[MAX_NAME_BYTES] = {0};

    MutexRefArray_t mra;
    mra.size = mutexCount;
    mra.mutexRefArray = malloc(mutexCount * sizeof(le_mutex_Ref_t));
    if (!mra.mutexRefArray)
    {
       LE_ERROR("mra.mutexRefArray is NULL");
       return NULL;
    }

    LE_INFO("Thread [%s] has started. Creating %ld mutexes.", le_thread_GetMyName(), mutexCount);

    le_mutex_Lock(MutexIndexMutexRef);
    long cnt = 0;
    while (cnt < mutexCount)
    {
        snprintf(mutexNameBuffer, MAX_NAME_BYTES, "[%s]Mutex%ld",
                 le_thread_GetMyName(), MutexCreateIdx);

        mra.mutexRefArray[cnt] = le_mutex_CreateNonRecursive(mutexNameBuffer);
        le_mutex_Lock(mra.mutexRefArray[cnt]);

        MutexCreateIdx++;
        cnt++;
    }
    le_mutex_Unlock(MutexIndexMutexRef);

    // Save the list of mutex refs in the thread's local storage.
    (void) pthread_setspecific(TsdMutexRefKey, &mra);

    le_sem_Post(SemaRef);

    le_event_RunLoop();
    return NULL;
}


// Create all mutexes spreaded evenly across the specified number of threads.
void createAllMutexes
(
    void
)
{
    char threadNameBuffer[MAX_THREAD_NAME_SIZE] = {0};

    long quotient = MutexNum / ThreadNum;
    long remainder = MutexNum % ThreadNum;
    long mutexPerThread;

    long threadCnt = 0;
    while (threadCnt < ThreadNum)
    {
        snprintf(threadNameBuffer, MAX_THREAD_NAME_SIZE, "Thread%ld", threadCnt);

        // Spread mutexes evenly among the threads, and put the remaining mutexes in the last thread.
        mutexPerThread = (threadCnt == (ThreadNum - 1)) ? (quotient + remainder) : quotient;

        // Store the thread references in an array
        ThreadRefArray[threadCnt] = le_thread_Create(threadNameBuffer, ThreadCreateMutex,
                                                     (void*)mutexPerThread);

        le_thread_Start(ThreadRefArray[threadCnt]);

        threadCnt++;
    }

    LE_INFO("========== Created all threads ===========");

    // waiting for all threads to finish creating their mutexes.
    long cnt = 0;
    while (cnt < ThreadNum)
    {
        le_sem_Wait(SemaRef);
        cnt++;
    }

    LE_INFO("========== All threads have created their mutexes ===========");
}


// Deleting mutexes of the specified range from the current thread.
// The range is specified such that, for a list of n items, Min is 1 and Max is n.
// In order to delete from x to (n - y) items, "offsetFromMin" is x - 1, and "offsetFromMax" is y.
// The offsets are distances from Min and Max, therefore they must be greater than 0.
// If they result in a range such that the lower bound is greater than the upper bound, no mutex is
// deleted.
static void DelMutexes
(
    long offsetFromMin,
    long offsetFromMax
)
{
    if ((offsetFromMin < 0) || (offsetFromMax < 0))
    {
        LE_WARN("DelMutexes bad params - negative offset(s).");
        return;
    }

    struct timespec sleepTime = {0, DelInv};

    MutexRefArray_t* mraRef = (MutexRefArray_t*)pthread_getspecific(TsdMutexRefKey);

    long idx = offsetFromMin;
    while (idx < (mraRef->size - offsetFromMax))
    {
        nanosleep(&sleepTime, NULL);
        le_mutex_Unlock(mraRef->mutexRefArray[idx]);
        idx++;
    }
}


// Delete from 1 to n-1 for all threads
static void DelMutex1toNMinus1PerThread
(
    void* param1,
    void* param2
)
{
    LE_INFO("DelMutex1toNMinus1PerThread in thread [%s]", le_thread_GetMyName());
    DelMutexes(0, 1);
}


// Delete all mutexes for the first thread.
static void DelAllMutexesFor1stThread
(
    void* param1,
    void* param2
)
{
    LE_INFO("DelAllMutexesFor1stThread in thread [%s]", le_thread_GetMyName());

    // Determine if this is the "1st" thread on the thread list. If so, delete all mutexes.
    if (le_thread_GetCurrent() == ThreadRefArray[0])
    {
        LE_INFO("This thread is the 1st thread in the thread list - deleting all mutexes.");
        DelMutexes(0, 0);
    }
}


// Delete all mutexes for a thread in the middle.
static void DelAllMutexesForMidThread
(
    void* param1,
    void* param2
)
{
    LE_INFO("DelAllMutexesForMidThread in thread [%s]", le_thread_GetMyName());

    long midIdx = ThreadNum / 2;

    // Determine if this is the "mid" thread on the thread list. If so, delete all mutexes.
    if (le_thread_GetCurrent() == ThreadRefArray[midIdx])
    {
        LE_INFO("This thread is the mid thread in the thread list - deleting all mutexes.");
        DelMutexes(0, 0);
    }
}


// wait for all threads to finish creating their mutexes, then ask them to do something.
void queueFuncToAllThreads
(
    le_event_DeferredFunc_t func
)
{
    long cnt = 0;
    while (cnt < ThreadNum)
    {
        le_event_QueueFunctionToThread(ThreadRefArray[cnt], func, NULL, NULL);
        cnt++;
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
    LE_ERROR("Usage: mutexFlux [TestWaitingList | TestRecursive]");
    LE_ERROR("       mutexFlux [1toN-1 | AllMutexes1stThread | AllMutexesMidThread | 1stThread |"
              " MidThread | None] [delete interval] [number of mutexes] [number of threads]");
    exit(EXIT_FAILURE);
}

static void DelInvArgHandler
(
    const char* arg
)
{
    DelInv = strtol(arg, NULL, 0);
}

static void NumMutexArgHandler
(
    const char* arg
)
{
    MutexNum = strtol(arg, NULL, 0);
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

    if ((strcmp(arg, "TestWaitingList") == 0) ||
        (strcmp(arg, "TestRecursive") == 0))
    {
        // do nothing
    }
    else if ((strcmp(arg, "1toN-1") == 0) ||
             (strcmp(arg, "AllMutexes1stThread") == 0) ||
             (strcmp(arg, "AllMutexesMidThread") == 0) ||
             (strcmp(arg, "1stThread") == 0) ||
             (strcmp(arg, "MidThread") == 0) ||
             (strcmp(arg, "None") == 0))
    {
        le_arg_AddPositionalCallback(DelInvArgHandler);
        le_arg_AddPositionalCallback(NumMutexArgHandler);
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
    // Create the key for thread specific data; ie. mutex refs.
    (void) pthread_key_create(&TsdMutexRefKey, NULL);

    // mutex for accessing the mutex index variable.
    MutexIndexMutexRef = le_mutex_CreateNonRecursive("MutexIndexMutex");

    // syncrhonizing among threads for locking/unlocking mutexes
    SemaRef = le_sem_Create("mutexFluxSemaphore", 0);

    // Initializing the array storing thread refs.
    ThreadRefArray = malloc(ThreadNum * sizeof(le_thread_Ref_t));
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
    else if (strcmp(TestType, "TestRecursive") == 0)
    {
        testRecursive();
    }
    else if (strcmp(TestType, "1toN-1") == 0)
    {
        createAllMutexes();
        queueFuncToAllThreads(DelMutex1toNMinus1PerThread);
    }
    else if (strcmp(TestType, "AllMutexes1stThread") == 0)
    {
        createAllMutexes();
        queueFuncToAllThreads(DelAllMutexesFor1stThread);
    }
    else if (strcmp(TestType, "AllMutexesMidThread") == 0)
    {
        createAllMutexes();
        queueFuncToAllThreads(DelAllMutexesForMidThread);
    }
    else if (strcmp(TestType, "1stThread") == 0)
    {
        createAllMutexes();
        LE_INFO("Deleting the 1st thread");
        le_thread_Cancel(ThreadRefArray[0]);
    }
    else if (strcmp(TestType, "MidThread") == 0)
    {
        createAllMutexes();
        LE_INFO("Deleting the middle thread");
        le_thread_Cancel(ThreadRefArray[ThreadNum / 2]);
    }
    else if (strcmp(TestType, "None") == 0)
    {
        createAllMutexes();
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

    LE_INFO("================== END of mutexFlux =====================");
}
