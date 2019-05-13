//--------------------------------------------------------------------------------------------------
/** @file microSupervisor/microSupervisor.c
 *
 * This is the Micro Supervisor application startup main thread.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "microSupervisor.h"

// Initializers are defined in these headers
#include "args.h"
#include "eventLoop.h"
#include "fileDescriptor.h"
#include "fs.h"
#include "json.h"
#include "log.h"
#include "mem.h"
#include "messaging.h"
#include "rand.h"
#include "safeRef.h"
#include "test.h"
#include "thread.h"
#include "timer.h"

/// Number of large entries in the argument string pool.
#define ARG_STRING_POOL_SIZE        LE_CONFIG_MAX_THREAD_POOL_SIZE
/// Number of bytes in a large argument string entry.
#define ARG_STRING_POOL_BYTES       240

/// Number of small entries in the argument string pool.
#define ARG_STRING_SMALL_POOL_SIZE  (ARG_STRING_POOL_SIZE * 2)
/// Number of bytes in a large argument string entry.
#define ARG_STRING_SMALL_POOL_BYTES ((ARG_STRING_POOL_BYTES + 16) / 4 - 16)

//--------------------------------------------------------------------------------------------------
/**
 * Calculate minimum of two values.
 *
 * @param   a   First value.
 * @param   b   Second value.
 *
 * @return Smallest value.
 */
//--------------------------------------------------------------------------------------------------
#define MIN(a, b)   ((a) < (b) ? (a) : (b))

//--------------------------------------------------------------------------------------------------
/**
 * Pool for argument strings.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ArgStringPoolRef;
LE_MEM_DEFINE_STATIC_POOL(ParentArgStringPool, ARG_STRING_POOL_SIZE, ARG_STRING_POOL_BYTES);

//--------------------------------------------------------------------------------------------------
/**
 * List of all apps on a system.
 *
 * @return The app list. The last item will be denoted by a NULL appNameStr.
 */
//--------------------------------------------------------------------------------------------------
extern const App_t *_le_supervisor_GetSystemApps
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize all services in system.
 */
//--------------------------------------------------------------------------------------------------
void _le_supervisor_InitAllServices
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Legato framework library.
 *
 * It initializes all the individual module in the framework in the correct order for RTOS.
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
static void InitLegatoFramework
(
    void
)
{
    // The order of initialization is important.
    rand_Init();
    mem_Init();
    log_Init();         // Uses memory pools.
    safeRef_Init();     // Uses memory pools and hash maps.
    mutex_Init();       // Uses memory pools.
    sem_Init();         // Uses memory pools.
    arg_Init();         // Uses memory pools.
    event_Init();       // Uses memory pools.
    timer_Init();       // Uses event loop.
    thread_Init();      // Uses event loop, memory pools and safe references.
    test_Init();        // Uses mutexes.
    msg_Init();         // Uses event loop.
    fs_Init();          // Uses memory pools and safe references and path manipulation.
    fd_Init();
    json_Init();

    // Init space for all services.
    _le_supervisor_InitAllServices();
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark a thread as exited in the app thread list
 */
//--------------------------------------------------------------------------------------------------
static void CleanupThread
(
    void* context
)
{
    TaskInfo_t *threadInfoPtr = context;

    // Free argument list
    if (threadInfoPtr->cmdlinePtr != NULL)
    {
        le_mem_Release(threadInfoPtr->cmdlinePtr);
    }

    threadInfoPtr->threadRef = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Duplicate a string using a memory pool.
 *
 * @return The duplicated string, or NULL if the allocation failed.
 */
//--------------------------------------------------------------------------------------------------
static char *PoolStrDup
(
    le_mem_PoolRef_t     poolRef,   ///< Pool to use for allocating the duplicate.
    const char          *str        ///< String to duplicate.
)
{
    char    *result;
    size_t   length = strlen(str);

    result = le_mem_TryVarAlloc(poolRef, length);
    if (result != NULL)
    {
        strncpy(result, str, length);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a task, given a pointer to the task structure.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartProc
(
    const App_t      *appPtr,
    const Task_t     *taskPtr,
    int               argc,
    const char      **argv,
    const char       *cmdlineStr
)
{
    int              i;
    int              taskNum = taskPtr - appPtr->taskList;
    le_result_t      result = LE_OK;
    le_thread_Ref_t  currentThread;
    TaskInfo_t      *taskInfoPtr;

    LE_DEBUG(" (%d) Creating task %s", taskNum, taskPtr->nameStr);

    taskInfoPtr = &appPtr->threadList[taskNum];
    memset(taskInfoPtr, 0, sizeof(*taskInfoPtr));
    LE_DEBUG("  +- taskInfo: %p", taskInfoPtr);

    if (cmdlineStr != NULL)
    {
        if (cmdlineStr[0] == '\0')
        {
            taskInfoPtr->argc = 1;
            taskInfoPtr->argv[0] = taskPtr->nameStr;
        }
        else
        {
            taskInfoPtr->cmdlinePtr = PoolStrDup(ArgStringPoolRef, cmdlineStr);
            if (taskInfoPtr->cmdlinePtr == NULL)
            {
                LE_WARN("Cannot create command line string for app '%s' task '%s'",
                    appPtr->appNameStr, taskPtr->nameStr);
                result = LE_NO_MEMORY;
                goto err;
            }

            taskInfoPtr->argc = MAX_ARGC;
            le_arg_Split(taskPtr->nameStr, taskInfoPtr->cmdlinePtr,
                &taskInfoPtr->argc, taskInfoPtr->argv);
        }
    }
    else
    {
        // It is assumed that argv and the strings it points to are long-lived.
        taskInfoPtr->argc = MIN(argc, MAX_ARGC - 1);
        memcpy(taskInfoPtr->argv, argv, taskInfoPtr->argc * sizeof(const char *));
    }

    LE_DEBUG("  +- with %d arguments:", taskInfoPtr->argc);
    for (i = 0; i < taskInfoPtr->argc; ++i)
    {
        LE_DEBUG("     %2d. '%s'", i, taskInfoPtr->argv[i]);
    }

    currentThread = le_thread_Create(taskPtr->nameStr, taskPtr->entryPoint, taskInfoPtr);
    if (!currentThread)
    {
        LE_WARN("Cannot create thread for app '%s' task '%s'",
                appPtr->appNameStr,
                taskPtr->nameStr);
        result = LE_NO_MEMORY;
        goto err;
    }
    taskInfoPtr->threadRef = currentThread;

    if (le_thread_SetPriority(currentThread, taskPtr->priority) != LE_OK)
    {
        LE_WARN("Failed to set priority (%d) for app '%s' task '%s'",
                taskPtr->priority,
                appPtr->appNameStr,
                taskPtr->nameStr);
        result = LE_FAULT;
        goto err;
    }

    if (taskPtr->stackPtr != NULL)
    {
        LE_ASSERT(taskPtr->stackSize > 0);

        if (le_thread_SetStack(currentThread, taskPtr->stackPtr, taskPtr->stackSize) != LE_OK)
        {
            LE_WARN("Failed to set stack for app '%s' task '%s'",
                    appPtr->appNameStr,
                    taskPtr->nameStr);
            result = LE_FAULT;
            goto err;
        }
    }
    else if (taskPtr->stackSize > 0)
    {
        if (le_thread_SetStackSize(currentThread, taskPtr->stackSize) != LE_OK)
        {
            LE_WARN("Failed to set stack size for app '%s' task '%s'",
                    appPtr->appNameStr,
                    taskPtr->nameStr);
            result = LE_FAULT;
            goto err;
        }
    }

    // Register function which will be called just before child thread exits
    le_thread_AddChildDestructor(currentThread,
                                 &CleanupThread,
                                 &appPtr->threadList[taskNum]);

    LE_DEBUG(" (%d) Starting task %s", taskNum, taskPtr->nameStr);

    le_thread_Start(currentThread);

    return LE_OK;

err:
    if (currentThread != NULL)
    {
        LE_CRIT("Allocated task %s has not been freed!", taskPtr->nameStr);
    }
    if (taskInfoPtr->cmdlinePtr != NULL)
    {
        le_mem_Release(taskInfoPtr->cmdlinePtr);
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Start an app, given a pointer to the app structure.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartApp
(
    const App_t* appPtr
)
{
    uint32_t taskNum;
    for (taskNum = 0; taskNum < appPtr->taskCount; ++taskNum)
    {
        const Task_t* currentTaskPtr = &appPtr->taskList[taskNum];

        le_result_t result = StartProc(appPtr, currentTaskPtr,
                                       currentTaskPtr->defaultArgc,
                                       currentTaskPtr->defaultArgv,
                                       NULL);
        if (result != LE_OK)
        {
            return result;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find an app, given an app name.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED const App_t* microSupervisor_FindApp
(
    const char* appNameStr       ///< Application name
)
{
    const App_t* currentAppPtr;

    // Iterate over all looking for the app to start.  App list is terminated by a NULL entry.
    for (currentAppPtr = _le_supervisor_GetSystemApps();
         currentAppPtr->appNameStr != NULL;
         ++currentAppPtr)
    {
        if (strcmp(appNameStr, currentAppPtr->appNameStr) == 0)
        {
            return currentAppPtr;
        }
    }

    // App not found
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find a task in an app, given the app reference and and the task name
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED const Task_t* microSupervisor_FindTask
(
    const App_t* appPtr,                 ///< [IN] pointer to the app
    const char* procNameStr              ///< [IN] task name
)
{
    uint32_t taskNum;

    for (taskNum = 0; taskNum < appPtr->taskCount; ++taskNum)
    {
        const Task_t* currentTaskPtr;

        currentTaskPtr = &appPtr->taskList[taskNum];
        if (strcmp(currentTaskPtr->nameStr, procNameStr) == 0)
        {
            return currentTaskPtr;
        }
    }

    // Task not found
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if an app is running.
 *
 * An app is running if at least one process in the app is running.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED bool microSupervisor_IsAppRunning
(
    const App_t* appPtr                 ///< [IN] pointer to the app
)
{
    // Check if there are any running tasks; if so app is already started.
    uint32_t task;
    for (task = 0; task < appPtr->taskCount; ++task)
    {
        if (appPtr->threadList[task].threadRef != NULL)
        {
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a task is running.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED bool microSupervisor_IsTaskRunning
(
    const App_t* appPtr,                 ///< [IN] pointer to the app
    const Task_t* taskPtr                ///< [IN] pointer to the task
)
{
    // Ensure task belongs to this app
    if ((taskPtr < appPtr->taskList) ||
        (taskPtr >= appPtr->taskList + appPtr->taskCount))
    {
        return false;
    }

    ptrdiff_t taskNum = taskPtr - appPtr->taskList;
    return (appPtr->threadList[taskNum].threadRef != NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Supervisor entry point.  Kick off all threads in all apps.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_microSupervisor_Main
(
    void
)
{
    const App_t         *currentAppPtr;
    le_mem_PoolRef_t     basePoolRef;

#if HAVE_PTHREAD_SETNAME
    pthread_setname_np(pthread_self(), __func__);
#endif

    InitLegatoFramework();

    basePoolRef = le_mem_InitStaticPool(ParentArgStringPool, ARG_STRING_POOL_SIZE,
        ARG_STRING_POOL_BYTES);
    ArgStringPoolRef = le_mem_CreateReducedPool(basePoolRef, "ArgStringPool",
        ARG_STRING_SMALL_POOL_SIZE, ARG_STRING_SMALL_POOL_BYTES);

    // Iterate over all apps.  App list is terminated by a NULL entry.
    for (currentAppPtr = _le_supervisor_GetSystemApps();
         currentAppPtr->appNameStr != NULL;
         ++currentAppPtr)
    {
        if (currentAppPtr->manualStart)
        {
            LE_DEBUG("Skipping manual start app %s", currentAppPtr->appNameStr);
            continue;
        }

        LE_DEBUG("Starting app %s", currentAppPtr->appNameStr);

        if (StartApp(currentAppPtr) != LE_OK)
        {
            LE_FATAL("Failed to start app '%s'", currentAppPtr->appNameStr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a specific app (by name)
 */
//--------------------------------------------------------------------------------------------------
le_result_t LE_SHARED le_microSupervisor_StartApp
(
    const char* appNameStr            ///< [IN] App name
)
{
    const App_t* currentAppPtr;

    // Iterate over all looking for the app to start.  App list is terminated by a NULL entry.
    for (currentAppPtr = _le_supervisor_GetSystemApps();
         currentAppPtr->appNameStr != NULL;
         ++currentAppPtr)
    {
        if (strcmp(appNameStr, currentAppPtr->appNameStr) == 0)
        {
            break;
        }
    }

    if (NULL == currentAppPtr->appNameStr)
    {
        // No such app
        LE_WARN("No app found named '%s'", appNameStr);
        return LE_NOT_FOUND;
    }

    if (microSupervisor_IsAppRunning(currentAppPtr))
    {
        LE_INFO("App '%s' is already running", appNameStr);
        return LE_OK;
    }

    return StartApp(currentAppPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Start a specific process (by name)
 */
//--------------------------------------------------------------------------------------------------
le_result_t LE_SHARED le_microSupervisor_RunProc
(
    const char* appNameStr,            ///< [IN] App name
    const char* procNameStr,           ///< [IN] Process name
    int argc,                          ///< [IN] Process argument count
    const char* argv[]                 ///< [IN] Process argument list
)
{
    const App_t* appPtr;
    const Task_t* taskPtr;

    appPtr = microSupervisor_FindApp(appNameStr);
    if (!appPtr)
    {
        // No such app
        LE_WARN("No app found named '%s'", appNameStr);
        return LE_NOT_FOUND;
    }

    taskPtr = microSupervisor_FindTask(appPtr, procNameStr);
    if (!taskPtr)
    {
        // No such task/process
        LE_WARN("No process found named '%s' in app '%s'", procNameStr, appNameStr);
        return LE_NOT_FOUND;
    }

    return StartProc(appPtr, taskPtr, argc, argv, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Start a specific process (by name).  The command line is passed as a single string.
 */
//--------------------------------------------------------------------------------------------------
le_result_t LE_SHARED le_microSupervisor_RunProcStr
(
    const char *appNameStr,     ///< App name.
    const char *procNameStr,    ///< Process name.
    const char *cmdlineStr      ///< Command line arguments.
)
{
    const App_t* appPtr;
    const Task_t* taskPtr;

    appPtr = microSupervisor_FindApp(appNameStr);
    if (!appPtr)
    {
        // No such app
        LE_WARN("No app found named '%s'", appNameStr);
        return LE_NOT_FOUND;
    }

    taskPtr = microSupervisor_FindTask(appPtr, procNameStr);
    if (!taskPtr)
    {
        // No such task/process
        LE_WARN("No process found named '%s' in app '%s'", procNameStr, appNameStr);
        return LE_NOT_FOUND;
    }

    return StartProc(appPtr, taskPtr, 0, NULL, cmdlineStr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Print app status on serial port (using printf).
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_microSupervisor_DebugAppStatus
(
    void
)
{
    const App_t* currentAppPtr;
    // Iterate over all looking for the app to start.  App list is terminated by a NULL entry.
    for (currentAppPtr = _le_supervisor_GetSystemApps();
         currentAppPtr->appNameStr != NULL;
         ++currentAppPtr)
    {
        printf("[%s] %s\n",
               (microSupervisor_IsAppRunning(currentAppPtr)?"running":"stopped"),
               currentAppPtr->appNameStr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the log level filter
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_microSupervisor_SetLogLevel
(
    le_log_Level_t level     ///< [IN] the filter level to set
)
{
    le_log_SetFilterLevel(level);
}
