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
#include "mem.h"
#include "safeRef.h"
#include "messaging.h"
#include "log.h"
#include "thread.h"
#include "eventLoop.h"
#include "timer.h"
#include "test.h"
#include "json.h"
#include "fs.h"
#include "custom_os/fd_device.h"

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

    // While microLegato is under development initialization of unimplemented modules is
    // commented out.  As the module is implemented, either add implementation for _Init()
    // function, or remove the init function if that module doesn't require initialization
    // on RTOS.
    arg_Init();
    mem_Init();
    log_Init();
    safeRef_Init();    // Uses memory pools and hash maps.
    mutex_Init();      // Uses memory pools.
    sem_Init();        // Uses memory pools.
    event_Init();      // Uses memory pools.
    timer_Init();      // Uses event loop.
    thread_Init();     // Uses event loop, memory pools and safe references.
    test_SystemInit(); // Uses mutexes.
    msg_Init();        // Uses event loop.
    fs_Init();         // Uses memory pools and safe references and path manipulation.
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
    TaskInfo_t** threadInfoPtrPtr = context;
    TaskInfo_t* threadInfoPtr = *threadInfoPtrPtr;
    int i;

    // Free argument list
    for (i = 1; i < threadInfoPtr->argc; ++i)
    {
        free((void*)threadInfoPtr->argv[i]);
    }

    // Free thread info and NULL out stored info.
    free(threadInfoPtr);

    *threadInfoPtrPtr = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Start a task, given a pointer to the task structure.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartProc
(
    const App_t* appPtr,
    const Task_t* taskPtr,
    int argc,
    const char* argv[]
)
{
    int taskNum = taskPtr - appPtr->taskList;
    int i;

    LE_DEBUG(" (%d) Creating task %s", taskNum, taskPtr->nameStr);

    // Create task info -- adding space for task name and terminating NULL.
    appPtr->threadList[taskNum] = malloc(sizeof(TaskInfo_t)
                                         + (argc+2)*sizeof(char*));

    LE_DEBUG("  +- taskInfo: %p", &appPtr->threadList[taskNum]);
    // Create argument list
    appPtr->threadList[taskNum]->argc = argc + 1;
    appPtr->threadList[taskNum]->argv[0] = taskPtr->nameStr;
    for (i = 1; i <= argc; ++i)
    {
        LE_DEBUG("       %d) '%s'", i, argv[i-1]);
        appPtr->threadList[taskNum]->argv[i] = strdup(argv[i-1]);
    }
    appPtr->threadList[taskNum]->argv[i] = NULL;

    LE_DEBUG("  +- with %d arguments:", appPtr->threadList[taskNum]->argc);
    for (i = 0; i < appPtr->threadList[taskNum]->argc; ++i)
    {
        LE_DEBUG("     %d. '%s'", i, appPtr->threadList[taskNum]->argv[i]);
    }

    appPtr->threadList[taskNum]->threadRef =
        le_thread_Create(taskPtr->nameStr, taskPtr->entryPoint, appPtr->threadList[taskNum]);

    le_thread_Ref_t currentThread = appPtr->threadList[taskNum]->threadRef;
    if (!currentThread)
    {
        LE_WARN("Cannot create thread for app '%s' task '%s'",
                appPtr->appNameStr,
                taskPtr->nameStr);
        return LE_NO_MEMORY;
    }


    if (le_thread_SetPriority(currentThread,
                              taskPtr->priority) != LE_OK)
    {
        LE_WARN("Failed to set priority (%d) for app '%s' task '%s'",
                taskPtr->priority,
                appPtr->appNameStr,
                taskPtr->nameStr);
        return LE_FAULT;
    }

    // If a stack size is set
    if (taskPtr->stackSize)
    {
        if (le_thread_SetStackSize(currentThread,
                                   taskPtr->stackSize) != LE_OK)
        {
            LE_WARN("Failed to set stack size for app '%s' task '%s'",
                    appPtr->appNameStr,
                    taskPtr->nameStr);
            return LE_FAULT;
        }
    }

    // Register function which will be called just before child thread exits
    le_thread_AddChildDestructor(currentThread,
                                 CleanupThread,
                                 &appPtr->threadList[taskNum]);

    LE_DEBUG(" (%d) Starting task %s", taskNum, taskPtr->nameStr);

    le_thread_Start(currentThread);

    return LE_OK;
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
    int taskNum;
    for (taskNum = 0; taskNum < appPtr->taskCount; ++taskNum)
    {
        const Task_t* currentTaskPtr = &appPtr->taskList[taskNum];

        le_result_t result = StartProc(appPtr, currentTaskPtr,
                                       currentTaskPtr->defaultArgc,
                                       currentTaskPtr->defaultArgv);
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
    int taskNum;

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
    int task;
    for (task = 0; task < appPtr->taskCount; ++task)
    {
        if (NULL != appPtr->threadList[task])
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
    return !!appPtr->threadList[taskNum];
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
    const App_t* currentAppPtr;

    pthread_setname_np(pthread_self(), __func__);

    InitLegatoFramework();

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

    return StartProc(appPtr, taskPtr, argc, argv);
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
