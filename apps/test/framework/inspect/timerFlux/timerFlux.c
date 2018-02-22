/*
 * This app creates and deletes timers according to the specified strategy and time interval.
 * The timers created are spreaded evenly across the specified number of threads.
 *
 * Note that since the "main" thread is always the "first thread" on the thread list, the options
 * (and therefore test cases) relevant to "1st thread" are more or less meaningless. I've included
 * them anyway for reference.
 */

// TODO: use usleep instead of nanosleep for delete interval

#include "legato.h"
#include "limit.h"
#include "thread.h"

#define MAX_THREADS 100
#define FIRST_ARG_SIZE 100

static long SleepIntervalNano;
static long ThreadNum;
static long TimerNum;
static le_thread_Ref_t* ThreadRefArray; // array storing references to the threads
/*
 * *** NOTE *** It's probably not really necessary to keep a global list of timer refs, since each
 * thread is keeping its timer refs in its thread local data storage.
 * This might be useful for a certain timer deletion patterm?  Feel free to remove this (and its
 * associates) if it's really useless.
 */
static le_timer_Ref_t* TimerRefArray; // array storing references to the timers.
// the index of the TimerRefArray at which the last timer creation has ended at.
static long TimerCreateIdx = 0;

LE_MUTEX_DECLARE_REF(MutexRef); // The mutex for accessing TimerRefArray.
le_sem_Ref_t SemaRef; // The semaphore for syncing timer creation and deletion between threads.

/*
 * Thread-specific data key, used to store timer refs. Note that a Legato thread already stores
 * timer refs for each thread, but there's no public API to access them.
 */
static pthread_key_t TsdKey;


// All settable attributes of a timer. This is the "settable attributes" part of timer_t.
typedef struct timerAttr
{
    char name[LIMIT_MAX_TIMER_NAME_BYTES];   ///< The timer name
    le_timer_ExpiryHandler_t handlerRef;     ///< Expiry handler function
    le_clk_Time_t interval;                  ///< Interval
    uint32_t repeatCount;                    ///< Number of times the timer will repeat
    void* contextPtr;                        ///< Context for timer expiry
}
TimerAttr_t;


// Reference type to TimerAttr_t
typedef struct timerAttr* TimerAttr_Ref_t;


// Timer expiry handler
static void TimerExpHandler
(
    le_timer_Ref_t timerRef
)
{
    // uncomment if needed. This can potentially flood the syslog.
    //LE_INFO("======== Timer expired...[%s] ======", le_thread_GetMyName());

    long long int* contextPtr = le_timer_GetContextPtr(timerRef);
    if (contextPtr != NULL)
    {
        LE_INFO("The first 8 byte of the Context is: %llx", *(long long int*)contextPtr);
    }
}


// The timer table stores different kinds of timers.
static TimerAttr_t TimerTable[] =
{
    // expires every 1000 secs, repeat infinitely
    {
        "T_repInf_1ks_",
        TimerExpHandler,
        {.sec = 1000, .usec = 0},
        0,
        NULL
    },
    // expires every 20 secs, repeat infinitely
    {
        "Timer_repInf_20s_",
        TimerExpHandler,
        {.sec = 20, .usec = 0},
        0,
        NULL
    },
    // expires every 30 secs, repeat infinitely
    {
        "Timer_repInf_30s_",
        TimerExpHandler,
        {.sec = 30, .usec = 0},
        0,
        NULL
    },
    // expires every 5 seconds, repeat once
    {
        "Timer_rep1_5s_",
        TimerExpHandler,
        {.sec = 5, .usec = 0},
        1,
        NULL
    },
    // expires every 7 seconds, repeat once
    {
        "Timer_rep1_7s_",
        TimerExpHandler,
        {.sec = 7, .usec = 0},
        1,
        NULL
    },
};


// Create a timer
static le_timer_Ref_t CreateTimer
(
    TimerAttr_Ref_t timerAttrRef
)
{
    char timerNameBuffer[LIMIT_MAX_TIMER_NAME_BYTES+20] = {0};

    snprintf(timerNameBuffer, sizeof(timerNameBuffer), "[%s]%s%ld",
             le_thread_GetMyName(), timerAttrRef->name, TimerCreateIdx);

    le_timer_Ref_t timerRef = le_timer_Create(timerNameBuffer);
    le_timer_SetHandler(timerRef, timerAttrRef->handlerRef);
    le_timer_SetInterval(timerRef, timerAttrRef->interval);
    le_timer_SetRepeat(timerRef, timerAttrRef->repeatCount);
    le_timer_SetContextPtr(timerRef, timerAttrRef->contextPtr);

    return timerRef;
}


// Data type for an array storing timer refs.
typedef struct TimerRefArray
{
    long size; // array size
    le_timer_Ref_t* timerRefArray;
}
TimerRefArray_t;


// This is the "main" function for each thread, which creates a series of timers.
static void* ThreadMain
(
    void* context
)
{
    long timerCount = (long)context;

    TimerRefArray_t tra;
    tra.size = timerCount;
    tra.timerRefArray = malloc(timerCount * sizeof(le_timer_Ref_t));
    LE_ASSERT(NULL != tra.timerRefArray);

    LE_INFO("Thread [%s] has started. Creating %ld timers.", le_thread_GetMyName(), timerCount);

    Lock();
    long cnt = 0;
    while (cnt < timerCount)
    {
        // Creating timer of the "1st" kind from the timer table
        TimerRefArray[TimerCreateIdx] = CreateTimer(&TimerTable[0]);
        le_timer_Start(TimerRefArray[TimerCreateIdx]);

        tra.timerRefArray[cnt] =  TimerRefArray[TimerCreateIdx];

        TimerCreateIdx++;
        cnt++;
    }
    Unlock();

    // Save the list of timer refs in the thread's local storage.
    (void) pthread_setspecific(TsdKey, &tra);

    le_sem_Post(SemaRef);

    le_event_RunLoop();
    return NULL;
}


// Create all timers spreaded evenly across the specified number of threads.
void createAllTimers
(
    void
)
{
    char threadNameBuffer[MAX_THREAD_NAME_SIZE] = {0};

    long quotient = TimerNum / ThreadNum;
    long remainder = TimerNum % ThreadNum;
    long timerPerThread;

    long threadCnt = 0;
    while (threadCnt < ThreadNum)
    {
        snprintf(threadNameBuffer, MAX_THREAD_NAME_SIZE, "Thread%ld", threadCnt);

        // Spread timers evenly among the threads, and put the remaining timers in the last thread.
        timerPerThread = (threadCnt == (ThreadNum - 1)) ? (quotient + remainder) : quotient;

        // Store the thread references in an array
        ThreadRefArray[threadCnt] = le_thread_Create(threadNameBuffer, ThreadMain,
                                                     (void*)timerPerThread);

        le_thread_Start(ThreadRefArray[threadCnt]);

        threadCnt++;
    }

    LE_INFO("========== Created all threads ===========");

    // waiting for all threads to finish creating their timers.
    long cnt = 0;
    while (cnt < ThreadNum)
    {
        le_sem_Wait(SemaRef);
        cnt++;
    }

    LE_INFO("========== All threads have created their timers ===========");
}


// Deleting timers of the specified range from the current thread.
// The range is specified such that, for a list of n items, Min is 1 and Max is n.
// In order to delete from x to (n - y) items, "offsetFromMin" is x - 1, and "offsetFromMax" is y.
// The offsets are distances from Min and Max, therefore they must be greater than 0.
// If they result in a range such that the lower bound is greater than the upper bound, no timer is
// deleted.
static void DelTimers
(
    long offsetFromMin,
    long offsetFromMax
)
{
    if ((offsetFromMin < 0) || (offsetFromMax < 0))
    {
        LE_WARN("DelTimers bad params - negative offset(s).");
        return;
    }

    struct timespec sleepTime = {0, SleepIntervalNano};

    TimerRefArray_t* traRef = (TimerRefArray_t*)pthread_getspecific(TsdKey);

    long idx = offsetFromMin;
    while (idx < (traRef->size - offsetFromMax))
    {
        nanosleep(&sleepTime, NULL);
        le_timer_Delete(traRef->timerRefArray[idx]);
        idx++;
    }
}

// Delete from 1 to n-1 for all threads
static void DelTimer1toNMinus1PerThread
(
    void* param1,
    void* param2
)
{
    LE_INFO("DelTimer1toNMinus1PerThread in thread [%s]", le_thread_GetMyName());
    DelTimers(0, 1);
}


// Delete all timers for the first thread.
static void DelAllTimersFor1stThread
(
    void* param1,
    void* param2
)
{
    LE_INFO("DelAllTimersFor1stThread in thread [%s]", le_thread_GetMyName());

    // Determine if this is the "1st" thread on the thread list. If so, delete all timers.
    if (le_thread_GetCurrent() == ThreadRefArray[0])
    {
        LE_INFO("This thread is the 1st thread in the thread list - deleting all timers.");
        DelTimers(0, 0);
    }
}


// Delete all timers for a thread in the middle.
static void DelAllTimersForMidThread
(
    void* param1,
    void* param2
)
{
    LE_INFO("DelAllTimersForMidThread in thread [%s]", le_thread_GetMyName());

    long midIdx = ThreadNum / 2;

    // Determine if this is the "mid" thread on the thread list. If so, delete all timers.
    if (le_thread_GetCurrent() == ThreadRefArray[midIdx])
    {
        LE_INFO("This thread is the mid thread in the thread list - deleting all timers.");
        DelTimers(0, 0);
    }
}


// wait for all threads to finish creating their timers, then ask them to do something.
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


COMPONENT_INIT
{
    const char* argDeleteStratPtr = le_arg_GetArg(0);
    const char* sleepIntervalNanoPtr = le_arg_GetArg(1);
    const char* timerNumPtr = le_arg_GetArg(2);
    const char* threadNumPtr = le_arg_GetArg(3);
    char argDeleteStrat[FIRST_ARG_SIZE];

    if (le_arg_NumArgs() != 4)
    {
        LE_ERROR("Usage: timerFlux [1toN-1 | AllTimers1stThread | AllTimersMidThread |"
                 " 1stThread | MidThread | None] [delete interval] [number of timers] [number of threads]");
        exit(EXIT_FAILURE);
    }

    (void) pthread_key_create(&TsdKey, NULL);

    MutexRef = le_mutex_CreateNonRecursive("timerFluxMutex");
    SemaRef = le_sem_Create("timerFluxSemaphore", 0);
    if (NULL == argDeleteStratPtr)
    {
        LE_ERROR("argDeleteStratPtr is NULL");
        exit(EXIT_FAILURE);
    }
    if (NULL == sleepIntervalNanoPtr)
    {
        LE_ERROR("sleepIntervalNanoPtr is NULL");
        exit(EXIT_FAILURE);
    }
    if (NULL == timerNumPtr)
    {
        LE_ERROR("timerNumPtr is NULL");
        exit(EXIT_FAILURE);
    }
    if (NULL == threadNumPtr)
    {
        LE_ERROR("threadNumPtr is NULL");
        exit(EXIT_FAILURE);
    }
    (void) snprintf(argDeleteStrat, FIRST_ARG_SIZE, "%s", argDeleteStratPtr);
    SleepIntervalNano = strtol(sleepIntervalNanoPtr, NULL, 0);
    TimerNum = strtol(timerNumPtr, NULL, 0);
    ThreadNum = strtol(threadNumPtr, NULL, 0);

    // Initializing the array storing timer refs.
    TimerRefArray = malloc(TimerNum * sizeof(le_timer_Ref_t));
    memset(TimerRefArray, 0, TimerNum * sizeof(le_timer_Ref_t));
    LE_ASSERT(NULL != TimerRefArray);

    if (ThreadNum <= MAX_THREADS)
    {
        // Initializing the array storing thread refs.
        ThreadRefArray = malloc(ThreadNum * sizeof(le_thread_Ref_t));
        memset(ThreadRefArray, 0, ThreadNum * sizeof(le_thread_Ref_t));
        LE_ASSERT(NULL != ThreadRefArray);
    }
    else
    {
        LE_ERROR("ThreadNum is greater then MAX_THREADS");
        exit(EXIT_FAILURE);
    }

    // Create/Delete timers, according to the defined strategy
    if (strcmp(argDeleteStrat, "1toN-1") == 0)
    {
        createAllTimers();
        queueFuncToAllThreads(DelTimer1toNMinus1PerThread);
    }
    else if (strcmp(argDeleteStrat, "AllTimers1stThread") == 0)
    {
        createAllTimers();
        queueFuncToAllThreads(DelAllTimersFor1stThread);
    }
    else if (strcmp(argDeleteStrat, "AllTimersMidThread") == 0)
    {
        createAllTimers();
        queueFuncToAllThreads(DelAllTimersForMidThread);
    }
    else if (strcmp(argDeleteStrat, "1stThread") == 0)
    {
        createAllTimers();
        LE_INFO("Deleting the 1st thread");
        le_thread_Cancel(ThreadRefArray[0]);
    }
    else if (strcmp(argDeleteStrat, "MidThread") == 0)
    {
        createAllTimers();
        LE_INFO("Deleting the middle thread");
        le_thread_Cancel(ThreadRefArray[ThreadNum / 2]);
    }
    else if (strcmp(argDeleteStrat, "None") == 0)
    {
        createAllTimers();
        LE_INFO("==== No timers deleted ====");
    }
    else
    {
        LE_ERROR("invalid timer delete strategy option.");
        exit(EXIT_FAILURE);
    }

    LE_INFO("========== FINISHED ===========");
}
