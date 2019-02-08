/**
 * @file appInfo.c
 *
 * App information service for RTOS platform.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"

#include "../microSupervisor.h"
#include "thread.h"

//--------------------------------------------------------------------------------------------------
/**
 * Gets the state of the specified application.  The state of unknown applications is STOPPED.
 *
 * @return
 *      The state of the specified application.
 *
 * @note If the application name pointer is null or if its string is empty or of bad format it is a
 *       fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_appInfo_State_t le_appInfo_GetState
(
    const char *appNameStr                 ///< Application name.
)
{
    const App_t* appPtr = microSupervisor_FindApp(appNameStr);

    if (appPtr &&
        microSupervisor_IsAppRunning(appPtr))
    {
        return LE_APPINFO_RUNNING;
    }
    else
    {
        return LE_APPINFO_STOPPED;
    }
}


// -------------------------------------------------------------------------------------------------
/**
 * Gets the state of the specified process in an application.  This function only works for
 * configured processes that the Supervisor starts directly.
 *
 * @return
 *      The state of the specified process.
 *
 * @note If the application or process names pointers are null or if their strings are empty or of
 *       bad format it is a fatal error, the function will not return.
 */
// -------------------------------------------------------------------------------------------------
le_appInfo_ProcState_t le_appInfo_GetProcState
(
    const char* appNameStr,       ///< Application name.
    const char* procNameStr      ///< Process name.
)
{
    const App_t* appPtr = microSupervisor_FindApp(appNameStr);
    const Task_t* taskPtr = (appPtr?microSupervisor_FindTask(appPtr, procNameStr):NULL);

    if (appPtr && taskPtr && microSupervisor_IsTaskRunning(appPtr, taskPtr))
    {
        return LE_APPINFO_PROC_RUNNING;
    }
    else
    {
        return LE_APPINFO_PROC_STOPPED;
    }
}

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


//-------------------------------------------------------------------------------------------------
/**
 * Gets the application name of the process with the specified PID.
 *
 * @return
 *      LE_OK if the application name was successfully found.
 *      LE_OVERFLOW if the application name could not fit in the provided buffer.
 *      LE_NOT_FOUND if the process is not part of an application.
 *      LE_FAULT if there was an error.
 */
//-------------------------------------------------------------------------------------------------
le_result_t le_appInfo_GetName
(
    int32_t  pid,           ///< [IN]  PID of the process.
    char    *appNameStr,    ///< [OUT] Application name buffer.
    size_t   appNameSize    ///< [IN]  Buffer size.
)
{
    const App_t *currentAppPtr;
    // Search for a task in the running task list

    // On FreeRTOS there is no difference between a task, a process, and a thread, so we look for a
    // pthread that matches the PID.  Note that children of a task do not inherit the PID for this
    // reason, so this implementation only finds matches for the "main" task of the each app's
    // "executables."

    for (currentAppPtr = _le_supervisor_GetSystemApps();
         currentAppPtr->appNameStr != NULL;
         ++currentAppPtr)
    {
        int task;

        for (task = 0; task < currentAppPtr->taskCount; ++task)
        {
            le_result_t result;
            pthread_t   thread;

            if (currentAppPtr->threadList[task] == NULL)
            {
                continue;
            }

            result = thread_GetOSThread(currentAppPtr->threadList[task]->threadRef, &thread);
            if (result == LE_OK && (int32_t) thread == pid)
            {
                return le_utf8_Copy(appNameStr, currentAppPtr->appNameStr, appNameSize, NULL);
            }
        }
    }

    return LE_NOT_FOUND;
}


//-------------------------------------------------------------------------------------------------
/**
 * Gets the application hash as a hexidecimal string.  The application hash is a unique hash of the
 * current version of the application.
 *
 * @return
 *      LE_OK if the application has was successfully retrieved.
 *      LE_OVERFLOW if the application hash could not fit in the provided buffer.
 *      LE_NOT_FOUND if the application is not installed.
 *      LE_FAULT if there was an error.
 *
 * @note If the application name pointer is null or if its string is empty or of bad format it is a
 *       fatal error, the function will not return.
 */
//-------------------------------------------------------------------------------------------------
le_result_t le_appInfo_GetHash
(
    const char* appNameStr,   ///< Application name.
    char* hashStr,             ///< Hash string.
    size_t hashSize            ///< Hash string size.
)
{
    return LE_NOT_IMPLEMENTED;
}


//-------------------------------------------------------------------------------------------------
/**
 * appInfo component initialization.
 *
 * As all data comes from the microSupervisor, no initialization is required.
 */
//-------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}
