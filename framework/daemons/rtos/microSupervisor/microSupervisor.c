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
#include "pathIter.h"
#include "rand.h"
#include "safeRef.h"
#include "test.h"
#include "thread.h"
#include "timer.h"
#include "fa/atomFile.h"

#include <locale.h>

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
#ifndef MIN
#  define MIN(a, b)   ((a) < (b) ? (a) : (b))
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Pool for argument strings.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ArgStringPoolRef;
LE_MEM_DEFINE_STATIC_POOL(ParentArgStringPool, ARG_STRING_POOL_SIZE, ARG_STRING_POOL_BYTES);


//--------------------------------------------------------------------------------------------------
/**
 * Active application group
 */
//--------------------------------------------------------------------------------------------------
static uint8_t ActiveRunGroup = 0;

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
    // Init locale for locale-dependent C functions
    setlocale(LC_ALL, "C");

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
    atomFile_Init();    // Uses memory pools.
    fs_Init();          // Uses memory pools and safe references and path manipulation.
    fd_Init();
#if LE_CONFIG_ENABLE_LE_JSON_API
    json_Init();
#endif
    pathIter_Init();    // Uses memory pools and safe references.

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
    else
    {
        for (int i = 1; i < threadInfoPtr->argc; ++i)
        {
            if (threadInfoPtr->argv[i] != NULL)
            {
                le_mem_Release((void*)threadInfoPtr->argv[i]);
            }
        }
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
    size_t   length = strlen(str) + 1;

    result = le_mem_TryVarAlloc(poolRef, length);
    if (result != NULL)
    {
        strncpy(result, str, length);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Determine if an app is in the active run group.
 *
 * Group 0 is always active; up to one additional run group may be active as well.
 */
//--------------------------------------------------------------------------------------------------
static bool InActiveRunGroup
(
    const App_t      *appPtr
)
{
    return !appPtr->runGroup || (appPtr->runGroup == ActiveRunGroup);
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
    const char       *cmdlineStr,
    bool             registerCleanUpDestructor
)
{
    int              i;
    int              taskNum = taskPtr - appPtr->taskList;
    le_result_t      result = LE_OK;
    le_thread_Ref_t  currentThread = NULL;
    TaskInfo_t      *taskInfoPtr;

    if (!InActiveRunGroup(appPtr))
    {
        LE_WARN("Not starting %s (group %d).  Active group is %d.",
                appPtr->appNameStr,
                appPtr->runGroup,
                ActiveRunGroup);

        return LE_BUSY;
    }

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
        taskInfoPtr->argc = MIN(argc + 1, MAX_ARGC - 1);

        // Set the program name as the first argument
        taskInfoPtr->argv[0] = taskPtr->nameStr;
        for (i = 1; i < taskInfoPtr->argc; ++i)
        {
            taskInfoPtr->argv[i] = PoolStrDup(ArgStringPoolRef, argv[i-1]);
        }
        taskInfoPtr->argv[taskInfoPtr->argc] = NULL;
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

    if (registerCleanUpDestructor)
    {
        // Register function which will be called just before child thread exits
        le_thread_AddChildDestructor(currentThread,
                                     &CleanupThread,
                                     &appPtr->threadList[taskNum]);
    }

    LE_DEBUG(" (%d) Starting task %s", taskNum, taskPtr->nameStr);

    // set a pid for threads created by this task to inherit
    thread_SetPidOnStart(currentThread);

    le_thread_SetJoinable(currentThread);

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

    if (!InActiveRunGroup(appPtr))
    {
        LE_WARN("Not starting %s (group %d).  Active group is %d.",
                appPtr->appNameStr,
                appPtr->runGroup,
                ActiveRunGroup);

        return LE_BUSY;
    }

    for (taskNum = 0; taskNum < appPtr->taskCount; ++taskNum)
    {
        const Task_t* currentTaskPtr = &appPtr->taskList[taskNum];

        le_result_t result = StartProc(appPtr, currentTaskPtr,
                                       currentTaskPtr->defaultArgc,
                                       currentTaskPtr->defaultArgv,
                                       NULL,
                                       false);
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
 * Get the active run group.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint8_t le_microSupervisor_GetActiveRunGroup
(
    void
)
{
    return ActiveRunGroup;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the active run group.
 *
 * If used, must be called before calling le_microSupervisor_Main()
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_microSupervisor_SetActiveRunGroup
(
    uint8_t runGroup
)
{
    // Ensure Legato has not been started
    LE_ASSERT(!ArgStringPoolRef);

    ActiveRunGroup = runGroup;
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

        if (!InActiveRunGroup(currentAppPtr))
        {
            LE_DEBUG("Skipping app in group %d (active group %d)",
                     currentAppPtr->runGroup,
                     ActiveRunGroup);
            continue;
        }

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

    return StartProc(appPtr, taskPtr, argc, argv, NULL, true);
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

    return StartProc(appPtr, taskPtr, 0, NULL, cmdlineStr, true);
}


//--------------------------------------------------------------------------------------------------
/**
 * Run a specific command (by name)
 */
//--------------------------------------------------------------------------------------------------
le_result_t LE_SHARED le_microSupervisor_RunCommand
(
    const char* appNameStr,            ///< [IN] App name
    const char* procNameStr,           ///< [IN] Process name
    int argc,                          ///< [IN] Process argument count
    const char* argv[]                 ///< [IN] Process argument list
)
{
    const App_t* appPtr;
    const Task_t* taskPtr;
    le_result_t result = LE_FAULT;

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

    int taskNum = taskPtr - appPtr->taskList;
    TaskInfo_t *taskInfoPtr = &appPtr->threadList[taskNum];
    void *retVal;

    result = StartProc(appPtr, taskPtr, argc, argv, NULL, true);

    // Immediately stop process once COMPONENT_INIT is processed
    if (result == LE_OK)
    {
        le_thread_Join(taskInfoPtr->threadRef, &retVal);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Run a CLI specific command
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t microSupervisor_RunCliCommand
(
    const char*             appNameStr,     ///< [IN] App name
    le_thread_MainFunc_t    entryPoint,     ///< [IN] Command's main entry point
    int                     argc,           ///< [IN] Command argument count
    const char*             argv[]          ///< [IN] Command argument list
)
{
    // Define task, stack and all necessary structures in order to run CLI commands
    // in dedicated thread.

    // Force static stacks as a workaround for LE-15486.
    // TODO: remove this once LE-15486 is fixed.
    #if LE_CONFIG_TARGET_HL78
    #    undef LE_CONFIG_STATIC_THREAD_STACKS
    #    define LE_CONFIG_STATIC_THREAD_STACKS 1
    #endif

    #if LE_CONFIG_STATIC_THREAD_STACKS
    LE_THREAD_DEFINE_STATIC_STACK(cli, LE_CONFIG_CLI_STACK_SIZE);
    #endif

    static const char* cli_Args[] =
    {
        NULL
    };

    static Task_t cliTask =
    {
        .nameStr = "cli",
        .priority = LE_THREAD_PRIORITY_MEDIUM,
    #if LE_CONFIG_STATIC_THREAD_STACKS
        .stackSize = sizeof(_thread_stack_cli),
        .stackPtr = _thread_stack_cli,
    #else
        .stackSize = 0,
        .stackPtr = NULL,
    #endif
        .entryPoint = NULL,
        .defaultArgc = 0,
        .defaultArgv = cli_Args,
        .watchdogTimeout = 0,
        .maxWatchdogTimeout = 0,
    };

    static TaskInfo_t cliTaskInfo;

    // Keep CLI task inside dummy application object. This object is updated
    // every call to reflect different CLI commands and then passed to StartProc().
    static App_t cliApp =
    {
        .taskCount = 1,
        .taskList = &cliTask,
        .threadList = &cliTaskInfo,
    };

    const App_t* appPtr;
    Task_t*      taskPtr;
    le_result_t  result;
    void*        retVal;

    le_arg_SetExitOnError(false);

    appPtr = microSupervisor_FindApp(appNameStr);
    if (!appPtr)
    {
        LE_WARN("No app found named '%s'", appNameStr);
        return LE_NOT_FOUND;
    }

    // Update application name to reflect current CLI command
    cliApp.appNameStr = appPtr->appNameStr;

    // Update CLI task's entry point to main function of current CLI command
    taskPtr = (Task_t*)cliApp.taskList;
    taskPtr->entryPoint = entryPoint;

    result = StartProc(&cliApp, taskPtr, argc, argv, NULL, true);

    if (result == LE_OK)
    {
        le_thread_Join(cliApp.threadList->threadRef, &retVal);
        result = (retVal) ? LE_FAULT : LE_OK;
    }
    return result;
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

//--------------------------------------------------------------------------------------------------
/**
* Get legato version string
*/
//--------------------------------------------------------------------------------------------------
LE_SHARED bool le_microSupervisor_GetLegatoVersion
(
    char*  bufferPtr,        ///< [OUT] output buffer to contain version string
    size_t size              ///< [IN] buffer length
)
{
    if (bufferPtr == NULL)
    {
        LE_ERROR("Version bufferPtr is NULL.");
        return false;
    }

    return le_utf8_Copy(bufferPtr, LE_VERSION, size, NULL) == LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns the corresponding task index for the specified thread
 */
//--------------------------------------------------------------------------------------------------
static int32_t GetAppTaskCount
(
    const App_t * pApp,     ///< [IN] the app to look through
    pthread_t threadId      ///< [IN] thread to find in the task list
)
{
    uint32_t task;
    for (task = 0; task < pApp->taskCount; ++task)
    {
        if (pApp->threadList[task].threadRef == NULL)
        {
            continue;
        }

        pthread_t currThreadId;
        if (LE_OK == thread_GetOSThread(pApp->threadList[task].threadRef, &currThreadId))
        {
            if (currThreadId == threadId)
            {
                return (int32_t) task;
            }
        }
    }

    return -1;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the configured max watchdog timeout if one exists
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED int32_t le_microSupervisor_GetMaxWatchdogTimeout
(
    pthread_t threadId ///< [IN] thread to find in the app list
)
{
    int32_t maxWatchdogTimeout = 0;
    int32_t taskCount = -1;
    const App_t* currentAppPtr;
    // Iterate over all looking for the app to start.  App list is terminated by a NULL entry.
    for (currentAppPtr = _le_supervisor_GetSystemApps();
         currentAppPtr->appNameStr != NULL;
         ++currentAppPtr)
    {
        taskCount = GetAppTaskCount(currentAppPtr, threadId);
        if (taskCount != -1)
        {
            if (currentAppPtr->maxWatchdogTimeout)
            {
                maxWatchdogTimeout = currentAppPtr->maxWatchdogTimeout;
            }

            if (currentAppPtr->taskList[taskCount].maxWatchdogTimeout)
            {
                maxWatchdogTimeout = currentAppPtr->taskList[taskCount].maxWatchdogTimeout;
            }

            return maxWatchdogTimeout;
        }
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
* Returns the configured default watchdog timeout if one exists
*/
//--------------------------------------------------------------------------------------------------
LE_SHARED int32_t le_microSupervisor_GetWatchdogTimeout
(
    pthread_t threadId ///< [IN] thread to find in the app list
)
{
    int32_t watchdogTimeout = 0;
    int32_t taskCount = -1;
    const App_t* currentAppPtr;
    // Iterate over all looking for the app to start.  App list is terminated by a NULL entry.
    for (currentAppPtr = _le_supervisor_GetSystemApps();
         currentAppPtr->appNameStr != NULL;
         ++currentAppPtr)
    {
        taskCount = GetAppTaskCount(currentAppPtr, threadId);
        if (taskCount != -1)
        {
            if (currentAppPtr->watchdogTimeout)
            {
                watchdogTimeout = currentAppPtr->watchdogTimeout;
            }

            if (currentAppPtr->taskList[taskCount].watchdogTimeout)
            {
                watchdogTimeout = currentAppPtr->taskList[taskCount].watchdogTimeout;
            }

            return watchdogTimeout;
        }
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
* Returns the configured manual start configuration
*/
//--------------------------------------------------------------------------------------------------
LE_SHARED bool le_microSupervisor_GetManualStart
(
    pthread_t threadId ///< [IN] thread to find in the app list
)
{
    int32_t taskCount = -1;
    const App_t* currentAppPtr;
    // Iterate over all looking for the app to start.  App list is terminated by a NULL entry.
    for (currentAppPtr = _le_supervisor_GetSystemApps();
         currentAppPtr->appNameStr != NULL;
         ++currentAppPtr)
    {
        taskCount = GetAppTaskCount(currentAppPtr, threadId);
        if (taskCount != -1)
        {
            return currentAppPtr->manualStart;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
* Returns task's name as defined in _le_supervisor_SystemApps
*/
//--------------------------------------------------------------------------------------------------
LE_SHARED const char* le_microSupervisor_GetTaskName
(
    pthread_t threadId ///< [IN] thread to find in the app list
)
{
    int32_t taskCount = -1;
    const App_t* currentAppPtr;
    // Iterate over all looking for the app to start.  App list is terminated by a NULL entry.
    for (currentAppPtr = _le_supervisor_GetSystemApps();
         currentAppPtr->appNameStr != NULL;
         ++currentAppPtr)
    {
        taskCount = GetAppTaskCount(currentAppPtr, threadId);
        if (taskCount != -1)
        {
            return currentAppPtr->taskList[taskCount].nameStr;
        }
    }
    return NULL;
}
