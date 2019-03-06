//--------------------------------------------------------------------------------------------------
/** @file supervisor/apps.c
 *
 * Module that handles all Legato applications within the Supervisor.  This module also handles all
 * app related IPC messages.
 *
 *  - @ref c_apps_applications
 *  - @ref c_apps_appProcs
 *
 * @section c_apps_applications Applications
 *
 * Apps run in containers. The container for an app is created either when someone calls
 * le_appCtrl_GetRef() or when the app is started, whichever comes first.
 * An app can be started by either an le_appCtrl_Start() IPC call or automatically on start-up
 * using the apps_AutoStart() API.
 *
 * When an app's container is created, a new app container object is created which contains a
 * list link, an app stop handler reference and the app object (which is also instantiated).  After
 * the app container object is created, it is placed on the list of inactive apps, waiting to be
 * started.  If there are errors in creating the container, the container will be destroyed and
 * an error will be reported in the log.
 *
 * When an inactive app is started, the app container is moved from the list of inactive apps to
 * the list of active apps.
 *
 * An app can be stopped by either an IPC call, a shutdown of the framework or when the app
 * terminates either normally or due to a fault action.
 *
 * The app's stop handler is set by the IPC handler and/or the fault monitor to take appropriate
 * actions when the app stops.  This is done because application stops are generally asynchronous.
 * For example, when an IPC commands an app to stop the IPC handler will set the app stop handler
 * then initiate the app stop by calling app_Stop().  However, the app may not stop right away
 * because all the processes in the app must first be killed and reaped.  The state of the app must
 * be checked within the SIGCHILD handler.  The SIGCHILD handler will then call the app stop handler
 * when the app has actually stopped.
 *
 * When an app has stopped it is popped off the active list and placed onto the inactive list of
 * apps.  When an app is restarted it is moved from the inactive list to the active list.  This
 * means we do not have to recreate app containers each time.  App containers are only cleaned when
 * the app is uninstalled.
 *
 * @section c_apps_appProcs Application Processes
 *
 * Generally the processes in an application are encapsulated and handled by the application class
 * in app.c.  However, to support command line control of processes within applications, references
 * to processes can be created and given to clients over the IPC API le_appProc.api.
 *
 * This API allows a client to get a reference to a configured process within an app, attached to
 * the process's standard streams, modify the process parameters (such as priority, etc.) and run
 * the process within the app.  Modifications to the process must not be persistent such that once
 * the client disconnects and the process is started normally the modified parameters are not used.
 * A configured process can only be reference by at most one client.
 *
 * The le_appProc.api also allows clients to create references to processes that are not configured
 * for the app.  This usage requires that the client provide an executable that is accessible by the
 * app.  The created process will run with default parameters (such as priority) unless specified by
 * the client.  These created processes are deleted as soon as the client disconnects so that when
 * the app is started normally only the configured processes are run.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "apps.h"
#include "app.h"
#include "interfaces.h"
#include "limit.h"
#include "wait.h"
#include "supervisor.h"
#include "sysPaths.h"
#include "properties.h"
#include "smack.h"
#include "cgroups.h"
#include "file.h"
#include "installer.h"

//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of all apps.
 *
 * If this entry in the config tree is missing or empty then no apps will be launched.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_APPS_LIST                  "apps"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the apps startManual value, used
 * to determine whether the app should be launched on system startup or if it should be
 * deferred for manual launch later.
 *
 * The startManual value is either true or false.  If true the app will not be launched on
 * startup.
 *
 * If this entry in the config tree is missing or is empty, automatic start will be used as the
 * default.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_START_MANUAL               "startManual"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that states whether the application is sandboxed or not
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_SANDBOXED                  "sandboxed"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the socket for the AppStop Server and Client.
 */
//--------------------------------------------------------------------------------------------------
#define APPSTOP_SERVER_SOCKET_NAME       LE_CONFIG_RUNTIME_DIR "/AppStopServer"


//--------------------------------------------------------------------------------------------------
/**
 * The file descriptors of the AppStop Server and Client sockets.
 */
//--------------------------------------------------------------------------------------------------
static int AppStopSvSocketFd = -1;


//--------------------------------------------------------------------------------------------------
/**
 * The fd monitor reference for the AppStop Server socket.
 */
//--------------------------------------------------------------------------------------------------
static le_fdMonitor_Ref_t AppStopSvSocketFdMonRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Handler to be called when all applications have shutdown.
 */
//--------------------------------------------------------------------------------------------------
static apps_ShutdownHandler_t AllAppsShutdownHandler = NULL;



struct AppContainer;


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for app stopped handler.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*AppStopHandler_t)
(
    struct AppContainer* appContainerPtr       ///< [IN] The app that stopped.
);


//--------------------------------------------------------------------------------------------------
/**
 * App object container.
 */
//--------------------------------------------------------------------------------------------------
typedef struct AppContainer
{
    app_Ref_t               appRef;       ///< Reference to the app.
    AppStopHandler_t        stopHandler;  ///< Handler function that gets called when the app stops.
    le_appCtrl_ServerCmdRef_t stopCmdRef; ///< Stores the reference to the command that requested
                                          ///< this app be stopped.  This reference must be sent in
                                          ///< the response to the stop app command.
    le_dls_Link_t           link;         ///< Link in the list of apps.
    bool                    isActive;     ///< true if the app is on the active list.  false if it
                                          ///< is on the inactive list.
    le_msg_SessionRef_t     clientRef;    ///< Reference to the client that has a reference to
                                          ///< this app. NULL if not connected client.
    le_appCtrl_TraceAttachHandlerFunc_t traceAttachHandler; ///< Client's trace attach handler.
    void* traceAttachContextPtr;          ///< Context for the client's trace attach handler.
    le_timer_Ref_t CheckAppStopTimer;     ///< Timer for waiting APP stop
    int AppStopTryCount;                  ///< Counter number for retrying to mark the stopped APP
}
AppContainer_t;


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for app containers.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AppContainerPool;


//--------------------------------------------------------------------------------------------------
/**
 * Safe reference map of applications.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t AppMap;


//--------------------------------------------------------------------------------------------------
/**
 * Safe reference map for application attach handlers.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t AppAttachHandlerMap;


//--------------------------------------------------------------------------------------------------
/**
 * List of all active app containers.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t ActiveAppsList = LE_DLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * List of all inactive app containers.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t InactiveAppsList = LE_DLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * Application Process object container.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    app_Proc_Ref_t      procRef;            ///< The process reference.
    AppContainer_t*     appContainerPtr;    ///< The app container reference.
    le_msg_SessionRef_t clientRef;          ///< Stores the reference to the client that created
                                            ///  this process.
}
AppProcContainer_t;


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for application process containers.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AppProcContainerPool;


//--------------------------------------------------------------------------------------------------
/**
 * Safe reference map of application processes.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t AppProcMap;

//--------------------------------------------------------------------------------------------------
/**
 * Timeout value for waiting processes to exit for an app.
 */
//--------------------------------------------------------------------------------------------------
static const le_clk_Time_t WaitAppStopTimeout =
{
    .sec = 0,
    .usec = 100*1000
};

//--------------------------------------------------------------------------------------------------
/**
 * Marking an app as "stopped". Since the mechanisms to determine app stop (cgroup release_agent)
 * and proc stop (SIGCHILD signals and the handlers) are decoupled, this function ensures that an
 * app is marked as stopped only when all configured processes have been marked as stopped.
 */
//--------------------------------------------------------------------------------------------------
static void MarkAppAsStopped
(
    void* param1Ptr,    ///< [IN] param 1, app ref
    void* param2Ptr     ///< [IN] param 2, app container ref
);

//--------------------------------------------------------------------------------------------------
/**
 * Deletes all application process containers for either an application or a client.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteAppProcs
(
    app_Ref_t appRef,                       ///< Apps to delete from. NULL if not used.
    le_msg_SessionRef_t clientRef           ///< Client to delete from.  NULL if not used.
)
{
    // Iterate over the safe references to find all application process containers for this client.
    le_ref_IterRef_t iter = le_ref_GetIterator(AppProcMap);

    while (le_ref_NextNode(iter) == LE_OK)
    {
        // Get the app process container.
        // WARNING: Casting away the const from le_ref_GetValue() and le_ref_GetSafeRef() so we can
        //          delete the data and the safe reference.  Should these functions be changed to
        //          not return a const value pointer?
        AppProcContainer_t* appProcContainerPtr = (AppProcContainer_t*)le_ref_GetValue(iter);

        LE_ASSERT(appProcContainerPtr != NULL);

        if ( ((appRef != NULL) && (appProcContainerPtr->appContainerPtr->appRef == appRef)) ||
             ((clientRef != NULL) && (appProcContainerPtr->clientRef == clientRef)) )
        {
            // Delete the safe reference.
            void* safeRef = (void*)le_ref_GetSafeRef(iter);
            LE_ASSERT(safeRef != NULL);

            le_ref_DeleteRef(AppProcMap, safeRef);

            // Delete the app proc.
            app_DeleteProc(appProcContainerPtr->appContainerPtr->appRef,
                           appProcContainerPtr->procRef);

            // Free the container.
            le_mem_Release(appProcContainerPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes application container and references to it.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteApp
(
    AppContainer_t* appContainerPtr     ///< App container to be deleted.
)
{
    le_ref_IterRef_t iter = le_ref_GetIterator(AppMap);

    while (le_ref_NextNode(iter) == LE_OK)
    {
        AppContainer_t* currAppContainerPtr = (AppContainer_t*)le_ref_GetValue(iter);

        if (appContainerPtr == currAppContainerPtr)
        {
            // Delete the safe reference.
            void* safeRef = (void*)le_ref_GetSafeRef(iter);
            LE_ASSERT(safeRef != NULL);

            le_ref_DeleteRef(AppMap, safeRef);

            // No need to look further since app names are unique.
            break;
        }
    }

    // Delete any app procs containers in this app.
    DeleteAppProcs(appContainerPtr->appRef, NULL);

    // Reset the additional link overrides here too because it is persistent in the file system.
    app_RemoveAllLinks(appContainerPtr->appRef);

    app_Delete(appContainerPtr->appRef);

    le_mem_Release(appContainerPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Puts the app into the inactive list.
 */
//--------------------------------------------------------------------------------------------------
static void DeactivateAppContainer
(
    AppContainer_t* appContainerPtr           ///< [IN] App to deactivate.
)
{
    le_dls_Remove(&ActiveAppsList, &(appContainerPtr->link));

    LE_INFO("Application '%s' has stopped.", app_GetName(appContainerPtr->appRef));

    appContainerPtr->stopHandler = NULL;

    le_dls_Queue(&InactiveAppsList, &(appContainerPtr->link));

    appContainerPtr->isActive = false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Restarts an application.
 */
//--------------------------------------------------------------------------------------------------
static void RestartApp
(
    AppContainer_t* appContainerPtr           ///< [IN] App to restart.
)
{
    // Always reset the stop handler so that when a process dies in the app that does not require
    // a restart it will be handled properly.
    appContainerPtr->stopHandler = DeactivateAppContainer;

    // Restart the app.
    if (app_Start(appContainerPtr->appRef) == LE_OK)
    {
        LE_INFO("Application '%s' restarted.", app_GetName(appContainerPtr->appRef));
    }
    else
    {
        LE_CRIT("Could not restart application '%s'.", app_GetName(appContainerPtr->appRef));

        DeactivateAppContainer(appContainerPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Responds to the stop app command.  Also deactivates the app container for the app that just
 * stopped.
 */
//--------------------------------------------------------------------------------------------------
static void RespondToStopAppCmd
(
    AppContainer_t* appContainerPtr           ///< [IN] App that stopped.
)
{
    // Save command reference for later use.
    void* cmdRef = appContainerPtr->stopCmdRef;

    DeactivateAppContainer(appContainerPtr);

    // Respond to the requesting process.
    le_appCtrl_StopRespond(cmdRef, LE_OK);
}


//--------------------------------------------------------------------------------------------------
/**
 * Shuts down the next running app.
 *
 * Deletes the current app container.
 */
//--------------------------------------------------------------------------------------------------
static void ShutdownNextApp
(
    AppContainer_t* appContainerPtr           ///< [IN] App that just stopped.
)
{
    LE_INFO("Application '%s' has stopped.", app_GetName(appContainerPtr->appRef));

    le_dls_Remove(&ActiveAppsList, &(appContainerPtr->link));

    DeleteApp(appContainerPtr);


    // Continue the shutdown process.
    apps_Shutdown();
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an active app container by application name.
 *
 * @return
 *      A pointer to the app container if successful.
 *      NULL if the app is not found.
 */
//--------------------------------------------------------------------------------------------------
static AppContainer_t* GetActiveApp
(
    const char* appNamePtr          ///< [IN] Name of the application to get.
)
{
    le_dls_Link_t* appLinkPtr = le_dls_Peek(&ActiveAppsList);

    while (appLinkPtr != NULL)
    {
        AppContainer_t* appContainerPtr = CONTAINER_OF(appLinkPtr, AppContainer_t, link);

        if (strncmp(app_GetName(appContainerPtr->appRef),
                                appNamePtr,
                                LIMIT_MAX_APP_NAME_BYTES) == 0)
        {
            return appContainerPtr;
        }

        appLinkPtr = le_dls_PeekNext(&ActiveAppsList, appLinkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an inactive app container by application name.
 *
 * @return
 *      A pointer to the app container if successful.
 *      NULL if the app is not found.
 */
//--------------------------------------------------------------------------------------------------
static AppContainer_t* GetInactiveApp
(
    const char* appNamePtr          ///< [IN] Name of the application to get.
)
{
    le_dls_Link_t* appLinkPtr = le_dls_Peek(&InactiveAppsList);

    while (appLinkPtr != NULL)
    {
        AppContainer_t* appContainerPtr = CONTAINER_OF(appLinkPtr, AppContainer_t, link);

        if (strncmp(app_GetName(appContainerPtr->appRef),
                                appNamePtr,
                                LIMIT_MAX_APP_NAME_BYTES) == 0)
        {
            return appContainerPtr;
        }

        appLinkPtr = le_dls_PeekNext(&InactiveAppsList, appLinkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the active app container for the app that has a process with the given PID.
 *
 * @return
 *      A pointer to the app container, if successful.
 *      NULL if the PID is not found.
 */
//--------------------------------------------------------------------------------------------------
static AppContainer_t* GetActiveAppWithProc
(
    pid_t pid
)
{
    le_dls_Link_t* appLinkPtr = le_dls_Peek(&ActiveAppsList);

    while (appLinkPtr != NULL)
    {
        AppContainer_t* appContainerPtr = CONTAINER_OF(appLinkPtr, AppContainer_t, link);

        if (app_HasTopLevelProc(appContainerPtr->appRef, pid))
        {
            return appContainerPtr;
        }

        appLinkPtr = le_dls_PeekNext(&ActiveAppsList, appLinkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create the app container if necessary.  This function searches for the app container in the
 * active and inactive lists first, if it can't find it then it creates the app container.
 *
 * @return
 *  - LE_OK if successful.
 *  - LE_NOT_FOUND if the app is not installed (no container created).
 *  - LE_FAULT if there was some other error (check logs).
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateApp
(
    const char* appNamePtr,          ///< [IN] Name of the application to launch.
    AppContainer_t** containerPtrPtr ///< [OUT] Ptr to the app container, or NULL if not created.
)
{
    // Check active list.
    *containerPtrPtr = GetActiveApp(appNamePtr);

    if (*containerPtrPtr != NULL)
    {
        return LE_OK;
    }

    // Check the inactive list.
    *containerPtrPtr = GetInactiveApp(appNamePtr);

    if (*containerPtrPtr != NULL)
    {
        return LE_OK;
    }

    // Get the configuration path for this app.
    char configPath[LIMIT_MAX_PATH_BYTES] = { 0 };

    if (le_path_Concat("/", configPath, LIMIT_MAX_PATH_BYTES,
                       CFG_NODE_APPS_LIST, appNamePtr, (char*)NULL) == LE_OVERFLOW)
    {
        LE_ERROR("App name configuration path '%s/%s' too large for internal buffers!",
                 CFG_NODE_APPS_LIST, appNamePtr);

        return LE_FAULT;
    }

    // Check that the app has a configuration value.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(configPath);

    if (le_cfg_IsEmpty(appCfg, ""))
    {
        LE_ERROR("Application '%s' is not installed.", appNamePtr);

        le_cfg_CancelTxn(appCfg);

        return LE_NOT_FOUND;
    }

    // Create the app object.
    app_Ref_t appRef = app_Create(configPath);

    if (appRef == NULL)
    {
        le_cfg_CancelTxn(appCfg);

        return LE_FAULT;
    }

    // Create the app container for this app.
    AppContainer_t* containerPtr = le_mem_ForceAlloc(AppContainerPool);

    containerPtr->appRef = appRef;
    containerPtr->link = LE_DLS_LINK_INIT;
    containerPtr->stopHandler = NULL;
    containerPtr->clientRef = NULL;
    containerPtr->traceAttachHandler = NULL;
    containerPtr->traceAttachContextPtr = NULL;
    containerPtr->CheckAppStopTimer = NULL;
    containerPtr->AppStopTryCount = 0;

    // Add this app to the inactive list.
    le_dls_Queue(&InactiveAppsList, &(containerPtr->link));
    containerPtr->isActive = false;

    le_cfg_CancelTxn(appCfg);

    *containerPtrPtr = containerPtr;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts an app.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartApp
(
    AppContainer_t* appContainerPtr         ///< [IN] App to start.
)
{
    le_dls_Remove(&InactiveAppsList, &(appContainerPtr->link));

    // Reset the running apps stop handler.
    appContainerPtr->stopHandler = DeactivateAppContainer;

    // Add the app to the active list.
    le_dls_Queue(&ActiveAppsList, &(appContainerPtr->link));
    appContainerPtr->isActive = true;

    // Start the app.

    le_result_t result = app_Start(appContainerPtr->appRef);

    switch(result)
    {
        // Fault action is to restart the app.
        case LE_TERMINATED:
            appContainerPtr->stopHandler = RestartApp;
            if (app_GetState(appContainerPtr->appRef) != APP_STATE_STOPPED)
            {
                // Stop the process. This is an asynchronous call that returns right away.
                app_Stop(appContainerPtr->appRef);
            }

            // If the application has already stopped then call its stop handler here.
            if (app_GetState(appContainerPtr->appRef) == APP_STATE_STOPPED)
            {
                appContainerPtr->stopHandler(appContainerPtr);
            }
            break;

        // Fault action is to stop the app.
        case LE_WOULD_BLOCK:
            appContainerPtr->stopHandler = DeactivateAppContainer;
            if (app_GetState(appContainerPtr->appRef) != APP_STATE_STOPPED)
            {
                // Stop the process. This is an asynchronous call that returns right away.
                app_Stop(appContainerPtr->appRef);
            }

            // If the application has already stopped then call its stop handler here.
            if (app_GetState(appContainerPtr->appRef) == APP_STATE_STOPPED)
            {
                appContainerPtr->stopHandler(appContainerPtr);
            }
            break;

        default:
            break;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Launch an app. Create the app container if necessary and start all the app's processes.
 *
 * @return
 *      LE_OK if successfully launched the app.
 *      LE_DUPLICATE if the app is already running.
 *      LE_NOT_FOUND if the app is not installed.
 *      LE_FAULT if the app could not be launched.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LaunchApp
(
    const char* appNamePtr      ///< [IN] Name of the application to launch.
)
{
    // Create the app.
    AppContainer_t* appContainerPtr;
    le_result_t result = CreateApp(appNamePtr, &appContainerPtr);
    if (result != LE_OK)
    {
        return result;
    }

    if (appContainerPtr->isActive)
    {
        LE_ERROR("Application '%s' is already running.", appNamePtr);
        return LE_DUPLICATE;
    }

    // Start the app.
    return StartApp(appContainerPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle application fault.  Gets the application fault action for the process that terminated
 * and handle the fault.
 *
 * @return
 *      LE_OK if the fault was handled.
 *      LE_FAULT if the fault could not be handled.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t HandleAppFault
(
    AppContainer_t* appContainerPtr,    ///< [IN] Application container reference.
    pid_t procPid,                      ///< [IN] Pid of the process that changed state.
    int procExitStatus                  ///< [IN] Return status of the process given by wait().
)
{
    // Get the fault action.
    FaultAction_t faultAction = FAULT_ACTION_IGNORE;

    app_SigChildHandler(appContainerPtr->appRef, procPid, procExitStatus, &faultAction);

    // Handle the fault.
    switch (faultAction)
    {
        case FAULT_ACTION_IGNORE:
            // Do nothing.
            break;

        case FAULT_ACTION_RESTART_APP:
            if (app_GetState(appContainerPtr->appRef) != APP_STATE_STOPPED)
            {
                // Stop the app if it hasn't already stopped.
                app_Stop(appContainerPtr->appRef);
            }

            // Set the handler to restart the app when the app stops.
            appContainerPtr->stopHandler = RestartApp;
            break;

        case FAULT_ACTION_STOP_APP:
            if (app_GetState(appContainerPtr->appRef) != APP_STATE_STOPPED)
            {
                // Stop the app if it hasn't already stopped.
                app_Stop(appContainerPtr->appRef);
            }
            break;

        case FAULT_ACTION_REBOOT:
            return LE_FAULT;

        default:
            LE_FATAL("Unexpected fault action %d.", faultAction);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes all application process containers for the client with the given session reference.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteClientAppProcs
(
    le_msg_SessionRef_t sessionRef,         ///< Session reference of the client.
    void*               contextPtr          ///< Not used.
)
{
    DeleteAppProcs(NULL, sessionRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes an inactive app object.
 */
//--------------------------------------------------------------------------------------------------
static void DeletesInactiveApp
(
    const char* appName,  ///< App being removed.
    void* contextPtr      ///< Context for this function.  Not used.
)
{
    // Find the app.
    AppContainer_t* appContainerPtr = GetInactiveApp(appName);

    if (appContainerPtr != NULL)
    {
        le_dls_Remove(&InactiveAppsList, &(appContainerPtr->link));

        // Delete the app object and container.
        DeleteApp(appContainerPtr);

        LE_DEBUG("Deleted app %s.", appName);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes all inactive app objects.
 */
//--------------------------------------------------------------------------------------------------
static void DeletesAllInactiveApp
(
    void
)
{
    le_dls_Link_t* appLinkPtr = le_dls_Pop(&InactiveAppsList);

    while (appLinkPtr != NULL)
    {
        AppContainer_t* appContainerPtr = CONTAINER_OF(appLinkPtr, AppContainer_t, link);

        // Delete the app object and container.
        DeleteApp(appContainerPtr);

        appLinkPtr = le_dls_Pop(&InactiveAppsList);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether an app's process is reference by any clients.
 */
//--------------------------------------------------------------------------------------------------
static bool IsAppProcAlreadyReferenced
(
    app_Proc_Ref_t appProcRef               ///< [IN] App process reference.
)
{
    // Iterate over the safe references to find all application process containers.
    le_ref_IterRef_t iter = le_ref_GetIterator(AppProcMap);

    while (le_ref_NextNode(iter) == LE_OK)
    {
        // Get the app process container.
        AppProcContainer_t* appProcContainerPtr = (AppProcContainer_t*)le_ref_GetValue(iter);

        LE_ASSERT(appProcContainerPtr != NULL);

        if (appProcContainerPtr->procRef == appProcRef)
        {
            return true;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks process name.
 */
//--------------------------------------------------------------------------------------------------
static bool IsProcNameValid
(
    const char* procNamePtr         ///< [IN] Process name.
)
{
    if ( (procNamePtr == NULL) || (strcmp(procNamePtr, "") == 0) )
    {
        LE_ERROR("Process name cannot be empty.");
        return false;
    }

    if (strstr(procNamePtr, "/") != NULL)
    {
        LE_ERROR("Process name contains illegal character '/'.");
        return false;
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks app name.
 */
//--------------------------------------------------------------------------------------------------
static bool IsAppNameValid
(
    const char* appNamePtr          ///< [IN] App name.
)
{
    if ( (appNamePtr == NULL) || (strcmp(appNamePtr, "") == 0) )
    {
        LE_ERROR("App name cannot be empty.");
        return false;
    }

    if (strstr(appNamePtr, "/") != NULL)
    {
        LE_ERROR("App name contains illegal character '/'.");
        return false;
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create the AppStop Server socket.
 */
//--------------------------------------------------------------------------------------------------
static int CreateAppStopSvSocket
(
    void
)
{
    struct sockaddr_un svaddr;
    int fd;

    fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    LE_FATAL_IF(fd == -1, "Error creating AppStop server socket.");

    LE_FATAL_IF(remove(APPSTOP_SERVER_SOCKET_NAME) == -1 && errno != ENOENT,
                "Error removing old AppStop server socket: " APPSTOP_SERVER_SOCKET_NAME);

    // Construct a well-known address and bind the socket to it
    memset(&svaddr, 0, sizeof(struct sockaddr_un));
    svaddr.sun_family = AF_UNIX;
    strncpy(svaddr.sun_path, APPSTOP_SERVER_SOCKET_NAME, sizeof(svaddr.sun_path) - 1);

    LE_FATAL_IF(bind(fd, (struct sockaddr*) &svaddr, sizeof(struct sockaddr_un)) == -1,
                "Error binding AppStop server socket.");

    return fd;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function called when there are configured procs in the proc lists, but
 * no actual running procs.
 */
//--------------------------------------------------------------------------------------------------
static void WaitAppStopHandler
(
    le_timer_Ref_t timerRef
)
{
    app_Ref_t appRef = (app_Ref_t)le_timer_GetContextPtr(timerRef);
    AppContainer_t* appContainerPtr = GetActiveApp(app_GetName(appRef));

    if (appContainerPtr == NULL)
    {
        // App may be missing in some fault cases when shutting down the system.
        // App has already been cleaned up, so safe to ignore shutdown notification.
        LE_WARN("Cannot find active app '%s'", app_GetName(appRef));
    }
    else
    {
        MarkAppAsStopped(appRef, appContainerPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Raise a timer to check if the App has stopped.
 */
//--------------------------------------------------------------------------------------------------
static void WaitAppStop
(
    void* param1Ptr,    ///< [IN] param 1, app ref
    void* param2Ptr     ///< [IN] param 2, app container ref
)
{
    app_Ref_t appRef = (app_Ref_t)param1Ptr;
    AppContainer_t* appContainerPtr = (AppContainer_t*)param2Ptr;

    if(appContainerPtr->CheckAppStopTimer == NULL)
    {
        char timerName[LIMIT_MAX_PATH_BYTES];
        snprintf(timerName, sizeof(timerName), "%s_CheckStop", app_GetName(appRef));
        appContainerPtr->CheckAppStopTimer = le_timer_Create(timerName);

        LE_ASSERT(le_timer_SetInterval(appContainerPtr->CheckAppStopTimer,
                                            WaitAppStopTimeout) == LE_OK);
        LE_ASSERT(le_timer_SetContextPtr(appContainerPtr->CheckAppStopTimer,
                                            (void*)appRef) == LE_OK);
        LE_ASSERT(le_timer_SetHandler(appContainerPtr->CheckAppStopTimer,
                                            WaitAppStopHandler) == LE_OK);
    }

    if (!le_timer_IsRunning(appContainerPtr->CheckAppStopTimer) &&
        appContainerPtr->CheckAppStopTimer != NULL)
    {
        LE_ASSERT(le_timer_Start(appContainerPtr->CheckAppStopTimer)
                                            == LE_OK);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete the check stop timer after the app has been stopped.
 */
//--------------------------------------------------------------------------------------------------
void WaitAppStopComplete
(
    void* paramPtr     ///< [IN] param, app container ref
)
{
    AppContainer_t* appContainerPtr = (AppContainer_t*)paramPtr;

    // Since the app has already stopped, we can stop the time-out timer now.
    if (appContainerPtr->CheckAppStopTimer != NULL)
    {
        le_timer_Delete(appContainerPtr->CheckAppStopTimer);
        appContainerPtr->CheckAppStopTimer = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Marking an app as "stopped". Since the mechanisms to determine app stop (cgroup release_agent)
 * and proc stop (SIGCHILD signals and the handlers) are decoupled, this function ensures that an
 * app is marked as stopped only when all configured processes have been marked as stopped.
 */
//--------------------------------------------------------------------------------------------------
static void MarkAppAsStopped
(
    void* param1Ptr,    ///< [IN] param 1, app ref
    void* param2Ptr     ///< [IN] param 2, app container ref
)
{
    #define MaxRetryCount 10
    // Note that this is a global retry counter shared by all apps.

    app_Ref_t appRef = (app_Ref_t)param1Ptr;
    AppContainer_t* appContainerPtr = (AppContainer_t*)param2Ptr;

    LE_FATAL_IF(appContainerPtr->AppStopTryCount > MaxRetryCount,
          "Cannot mark app as stopped because configured procs' states can't be marked as stopped");

    if (app_HasConfRunningProc(appRef))
    {
        // If there are configured procs in the proc lists but no actual running procs, we might be
        // in a race condition with the sigchild handlers. Re-queue this function and let the try
        // again later, hopefully at that time the sigchild handlers have run and set the proc state
        // correctly, then we can proceed to set the app state as stopped.
        if (cgrp_IsEmpty(CGRP_SUBSYS_FREEZE, app_GetName(appRef)))
        {
            appContainerPtr->AppStopTryCount++;
            LE_WARN("App %s still has configured running procs. Cannot yet mark app as stopped.",
                    app_GetName(appRef));
            le_event_QueueFunction(WaitAppStop, appRef, appContainerPtr);
        }
        // If there are configured procs in the proc lists and there are actual running procs, then
        // we are in the middle of fault action "restart" which restarts the faulty process while
        // keeping the app running. Therefore do not mark the app as stopped.
        else
        {
            appContainerPtr->AppStopTryCount = 0;
            LE_DEBUG("Fault action 'restart' in action. Not marking app as stopped.");
        }
    }
    else
    {
        appContainerPtr->AppStopTryCount = 0;

        // If there are no configured procs in the proc lists and there are no actual running procs,
        // then the app has stopped. We can proceed to mark the app as stopped.
        if (cgrp_IsEmpty(CGRP_SUBSYS_FREEZE, app_GetName(appRef)))
        {
            WaitAppStopComplete(appContainerPtr);
            app_StopComplete(appRef);
            appContainerPtr->stopHandler(appContainerPtr);
        }
        // If there are no configured procs in the proc lists but there are actual running procs,
        // then this is an unexpected scenario. Maybe the cgroups "notify on release" behaviour has
        // changed.
        else
        {
            LE_FATAL("Unexpected scenario. Cgroups notify_on_release might not work as expected.");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function called when the last process has exited a freezer cgroup.
 */
//--------------------------------------------------------------------------------------------------
static void AppStopHandler
(
    int fd,         ///< [IN] fd being monitored.
    short events    ///< [IN] events that happened.
)
{
    if (events & POLLIN)
    {
        ssize_t numBytesRead;
        char appName[LIMIT_MAX_APP_NAME_BYTES] = {0};

        do
        {
            numBytesRead = recvfrom(fd, appName, LIMIT_MAX_APP_NAME_BYTES, 0, NULL, NULL);
        }
        while ((numBytesRead == -1) && (errno == EINTR));

        if (numBytesRead > 0)
        {
            AppContainer_t* appContainerPtr = GetActiveApp(appName);
            if (appContainerPtr == NULL)
            {
                // App may be missing in some fault cases when shutting down the system.
                // App has already been cleaned up, so safe to ignore shutdown notification.
                LE_WARN("Cannot find active app '%s'", appName);
            }
            else
            {
                app_Ref_t appRef = appContainerPtr->appRef;

                MarkAppAsStopped(appRef, appContainerPtr);
            }
        }
        else if (numBytesRead == 0)
        {
            LE_FATAL("No app name sent; therefore cannot determine which app to stop.");
        }
        else
        {
            LE_FATAL("Error reading from the AppStop server socket, %m");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Release an application reference.
 */
//--------------------------------------------------------------------------------------------------
static void ReleaseAppRef
(
    void* appSafeRef,                       ///< [IN] Safe reference for the app.
    AppContainer_t* appContainerPtr         ///< [IN] App container the safe reference points to.
)
{
    // Reset the overrides.
    app_SetRunForAllProcs(appContainerPtr->appRef, true);
    app_RemoveAllLinks(appContainerPtr->appRef);
    app_SetBlockCallback(appContainerPtr->appRef, NULL, NULL);

    // Remove the safe ref.
    le_ref_DeleteRef(AppMap, appSafeRef);

    appContainerPtr->clientRef = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes all application process containers for either an application or a client.
 */
//--------------------------------------------------------------------------------------------------
static void ReleaseClientAppRefs
(
    le_msg_SessionRef_t sessionRef,         ///< Session reference of the client.
    void*               contextPtr          ///< Not used.
)
{
    // Iterate over the safe references to find all application containers for this client.
    le_ref_IterRef_t iter = le_ref_GetIterator(AppMap);

    while (le_ref_NextNode(iter) == LE_OK)
    {
        // Get the app container.
        AppContainer_t* appContainerPtr = (AppContainer_t*)le_ref_GetValue(iter);

        LE_ASSERT(appContainerPtr != NULL);

        if (appContainerPtr->clientRef == sessionRef)
        {
            void* safeRef = (void*)le_ref_GetSafeRef(iter);
            LE_ASSERT(safeRef != NULL);

            ReleaseAppRef(safeRef, appContainerPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Process block handler.  Called when a process has blocked on startup.
 */
//--------------------------------------------------------------------------------------------------
static void ProcBlockHandler
(
    pid_t pid,                          ///< [IN] PID of the blocked process.
    const char* procNamePtr,            ///< [IN] Name of the blocked process.
    void* appSafeRef                    ///< [IN] Safe ref app pointer.
)
{
    AppContainer_t* appContainerPtr = le_ref_Lookup(AppMap, appSafeRef);

    LE_FATAL_IF(appContainerPtr == NULL, "Invalid application reference.");

    if (appContainerPtr->traceAttachHandler != NULL)
    {
        appContainerPtr->traceAttachHandler(appSafeRef, pid, procNamePtr,
                                            appContainerPtr->traceAttachContextPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the applications system.
 */
//--------------------------------------------------------------------------------------------------
void apps_Init
(
    void
)
{
    app_Init();

    // Create memory pools.
    AppContainerPool = le_mem_CreatePool("appContainers", sizeof(AppContainer_t));
    AppProcContainerPool = le_mem_CreatePool("appProcContainers", sizeof(AppProcContainer_t));

    AppProcMap = le_ref_CreateMap("AppProcs", 5);
    AppMap = le_ref_CreateMap("App", 5);
    AppAttachHandlerMap = le_ref_CreateMap("AppAttachHandlers", 5);

    le_instStat_AddAppUninstallEventHandler(DeletesInactiveApp, NULL);
    le_instStat_AddAppInstallEventHandler(DeletesInactiveApp, NULL);

    le_msg_AddServiceCloseHandler(le_appProc_GetServiceRef(), DeleteClientAppProcs, NULL);

    le_msg_AddServiceCloseHandler(le_appCtrl_GetServiceRef(), ReleaseClientAppRefs, NULL);

    // Setup sockets to notify Supervisor when an app stops.
    AppStopSvSocketFd = CreateAppStopSvSocket();
    AppStopSvSocketFdMonRef = le_fdMonitor_Create("AppStopSvSocketFdMon", AppStopSvSocketFd,
                                                  AppStopHandler, POLLIN);

    // Specify the program to be run when the last process exits a freezer sub-group. This program
    // notifies the Supervisor which app has stopped.
    file_WriteStr("/sys/fs/cgroup/freezer/release_agent",
                  "/legato/systems/current/bin/_appStopClient", 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initiates the shut down of all the applications.  The shut down sequence happens asynchronously.
 * A shut down handler should be set using apps_SetShutdownHandler() to be
 * notified when all applications actually shut down.
 */
//--------------------------------------------------------------------------------------------------
void apps_Shutdown
(
    void
)
{
    // Deletes all inactive apps first.
    DeletesAllInactiveApp();

    // Get the first app to stop.
    le_dls_Link_t* appLinkPtr = le_dls_Peek(&ActiveAppsList);

    if (appLinkPtr != NULL)
    {
        AppContainer_t* appContainerPtr = CONTAINER_OF(appLinkPtr, AppContainer_t, link);

        // Set the stop handler that will continue to stop all apps and the framework.
        appContainerPtr->stopHandler = ShutdownNextApp;

        // Stop the first app.  This will kick off the chain of callback handlers that will stop
        // all apps.
        app_Stop(appContainerPtr->appRef);

        // If the application has already stopped then call its stop handler here.  Otherwise the
        // stop handler will be called from the AppStopHandler() when the app actually stops.
        if (app_GetState(appContainerPtr->appRef) == APP_STATE_STOPPED)
        {
            appContainerPtr->stopHandler(appContainerPtr);
        }
    }
    else
    {
        le_fdMonitor_Delete(AppStopSvSocketFdMonRef);

        close(AppStopSvSocketFd);

        if (AllAppsShutdownHandler != NULL)
        {
            AllAppsShutdownHandler();
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the shutdown handler to be called when all the applications have shutdown.
 */
//--------------------------------------------------------------------------------------------------
void apps_SetShutdownHandler
(
    apps_ShutdownHandler_t shutdownHandler      ///< [IN] Shut down handler.  Can be NULL.
)
{
    AllAppsShutdownHandler = shutdownHandler;
}


//--------------------------------------------------------------------------------------------------
/**
 * Start all applications marked as 'auto' start.
 */
//--------------------------------------------------------------------------------------------------
void apps_AutoStart
(
    void
)
{
    // Read the list of applications from the config tree.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(CFG_NODE_APPS_LIST);

    if (le_cfg_GoToFirstChild(appCfg) != LE_OK)
    {
        LE_WARN("No applications installed.");

        le_cfg_CancelTxn(appCfg);

        return;
    }

    do
    {
        // Check the start mode for this application.
        if (!le_cfg_GetBool(appCfg, CFG_NODE_START_MANUAL, false))
        {
            // Get the app name.
            char appName[LIMIT_MAX_APP_NAME_BYTES];

            if (le_cfg_GetNodeName(appCfg, "", appName, sizeof(appName)) == LE_OVERFLOW)
            {
                LE_ERROR("AppName buffer was too small, name truncated to '%s'.  "
                         "Max app name in bytes, %d.  Application not launched.",
                         appName, LIMIT_MAX_APP_NAME_BYTES);
            }
            else
            {
                // Launch the application now.  No need to check the return code because there is
                // nothing we can do about errors.
                LaunchApp(appName);
            }
        }
    }
    while (le_cfg_GoToNextSibling(appCfg) == LE_OK);

    le_cfg_CancelTxn(appCfg);
}


//--------------------------------------------------------------------------------------------------
/**
 * The SIGCHLD handler for the applications.  This should be called from the Supervisor's SIGCHILD
 * handler.
 *
 * @note
 *      This function will reap the child if the child is a configured application process,
 *      otherwise the child will be reaped by the Supervisor's SIGCHILD handler.
 *
 * @return
 *      LE_OK if the signal was handled without incident.
 *      LE_NOT_FOUND if the pid is not a configured application process.  The child will not be
 *      reaped.
 *      LE_FAULT if the signal indicates a failure of one of the applications which requires a
 *               system restart.
 */
//--------------------------------------------------------------------------------------------------
le_result_t apps_SigChildHandler
(
    pid_t pid               ///< [IN] Pid of the process that produced the SIGCHLD.
)
{
    AppContainer_t* appContainerPtr = GetActiveAppWithProc(pid);

    if (appContainerPtr == NULL)
    {
        return LE_NOT_FOUND;
    }

    // This child process is an application process.
    // Reap the child now.
    int status = wait_ReapChild(pid);

    // Handle any faults that the child process state change my have caused.
    return HandleAppFault(appContainerPtr, pid, status);
}


//--------------------------------------------------------------------------------------------------
/**
 * Verify that all devices in our sandboxed applications match with the device outside the sandbox.
 * Remove devices and allow supervisor to recreate them.
 */
//--------------------------------------------------------------------------------------------------
void apps_VerifyAppWriteableDeviceFiles
(
    void
)
{
    // Read the list of applications from the config tree.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(CFG_NODE_APPS_LIST);

    if (le_cfg_GoToFirstChild(appCfg) != LE_OK)
    {
        LE_WARN("No applications installed.");

        le_cfg_CancelTxn(appCfg);

        return;
    }

    do
    {
        // Get the app name.
        char appName[LIMIT_MAX_APP_NAME_BYTES];

        if (le_cfg_GetNodeName(appCfg, "", appName, sizeof(appName)) == LE_OVERFLOW)
        {
            LE_ERROR("AppName buffer was too small, name truncated to '%s'.  "
                     "Max app name in bytes, %d.  Application not launched.",
                     appName, LIMIT_MAX_APP_NAME_BYTES);
        }
        else
        {
            // Only check if application is sandboxed since included devices are created as new
            // device nodes
            if (le_cfg_GetBool(appCfg, CFG_NODE_SANDBOXED, true))
            {
                // Get the app hash
                char versionBuffer[LIMIT_MAX_APP_HASH_LEN] = "";

                if (le_appInfo_GetHash(appName, versionBuffer, sizeof(versionBuffer)) != LE_OK)
                {
                    LE_ERROR("Unable to retrieve application '%s' hash", appName);
                }
                else
                {
                    installer_RemoveAppWriteableDeviceFiles("current", versionBuffer, appName);
                }
            }
        }
    }
    while (le_cfg_GoToNextSibling(appCfg) == LE_OK);

    le_cfg_CancelTxn(appCfg);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to an application. Has the side-effect of creating the app's runtime container
 * if it hasn't already been created.
 *
 * @return
 *      Reference to the named app.
 *      NULL on error.
 *
 * @warning No more than one app can hold a reference at any given time.
 *
 * @todo Remove side-effect of creating an app container.  App container creation shouldn't be
 *       tied to this.  The container isn't needed until the app starts, so it could be done then
 *       instead.
 */
//--------------------------------------------------------------------------------------------------
static void* appCtrl_GetRef
(
    const char* appName                     ///< [IN] Name of the app to get the ref for.
)
{
    if (!IsAppNameValid(appName))
    {
        LE_ERROR("Invalid app name.");
        return NULL;
    }

    // Get a ref for an app with the app name.
    AppContainer_t* appContainerPtr;
    le_result_t result = CreateApp(appName, &appContainerPtr);
    if (result != LE_OK)
    {
        return NULL;
    }

    // Check if someone is already holding a reference to this app.
    if (appContainerPtr->clientRef != NULL)
    {
        LE_ERROR("Application '%s' is already referenced by a client.", appName);
        return NULL;
    }

    void* appSafeRef = le_ref_CreateRef(AppMap, appContainerPtr);

    // Store the client reference.
    appContainerPtr->clientRef = le_appCtrl_GetClientSessionRef();

    return appSafeRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Release the reference to an application.
 */
//--------------------------------------------------------------------------------------------------
static void appCtrl_ReleaseRef
(
    le_appCtrl_AppRef_t appRef          ///< [IN] Ref to the app.
)
{
    AppContainer_t* appContainerPtr = le_ref_Lookup(AppMap, appRef);

    if (appContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application reference.");
        return;
    }

    ReleaseAppRef(appRef, appContainerPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the run flag for a process in an application.
 *
 * If there is an error this function will kill the calling client.
 */
//--------------------------------------------------------------------------------------------------
static void appCtrl_SetRun
(
    le_appCtrl_AppRef_t appRef,         ///< [IN] Ref to the app.
    const char* procName,               ///< [IN] Process name to set the run flag for.
    bool run                            ///< [IN] Flag to run the process or not.
)
{
    if (!IsProcNameValid(procName))
    {
        LE_KILL_CLIENT("Invalid process name.");
        return;
    }

    AppContainer_t* appContainerPtr = le_ref_Lookup(AppMap, appRef);

    if (appContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application reference.");
        return;
    }

    // Look up the proc ref by name.
    app_Proc_Ref_t procContainerPtr = app_GetProcContainer(appContainerPtr->appRef, procName);

    if (procContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid process name '%s'.", procName);
        return;
    }

    app_SetRun(procContainerPtr, run);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the debug flag for a process in an application.
 *
 * If there is an error this function will kill the calling client.
 */
//--------------------------------------------------------------------------------------------------
static void appCtrl_SetDebug
(
    le_appCtrl_AppRef_t appRef,         ///< [IN] Ref to the app.
    const char* procName,               ///< [IN] Process name to set the run flag for.
    bool debug                          ///< [IN] Flag to debug the process or not.
)
{
    if (!IsProcNameValid(procName))
    {
        LE_KILL_CLIENT("Invalid process name.");
        return;
    }

    AppContainer_t* appContainerPtr = le_ref_Lookup(AppMap, appRef);

    if (appContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application reference.");
        return;
    }

    // Look up the proc ref by name.
    app_Proc_Ref_t procContainerPtr = app_GetProcContainer(appContainerPtr->appRef, procName);

    if (procContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid process name '%s'.", procName);
        return;
    }

    app_SetDebug(procContainerPtr, debug);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts an app.  This function is called by the event loop when a separate process requests to
 * start an app.
 *
 * @return
 *      LE_OK if the app is successfully started.
 *      LE_DUPLICATE if the app is already running.
 *      LE_NOT_FOUND if the app is not installed.
 *      LE_FAULT if there was an error and the app could not be launched.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t appCtrl_Start
(
    const char* appName                 ///< [IN] Name of the application to start.
)
{
    if (!IsAppNameValid(appName))
    {
        LE_KILL_CLIENT("Invalid app name.");
        return LE_FAULT;
    }

    LE_DEBUG("Received request to start application '%s'.", appName);

    return LaunchApp(appName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops an app. This function is called by the event loop when a separate process requests to stop
 * an app.
 *
 * @note If this function returns LE_OK that does not mean the app has necessarily stopped yet
 *       because stopping apps is asynchronous.  When the app actually stops the stopHandler will be
 *       called.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the app could not be found.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t appCtrl_Stop
(
    le_appCtrl_ServerCmdRef_t cmdRef,       ///< [IN] Command reference that must be passed to this
                                            ///       command's response function.
    const char* appName,                ///< [IN] Name of the application to stop.
    AppStopHandler_t stopHandler        ///< [IN] Handler to use to report when the app has stopped.
)
{
    if (!IsAppNameValid(appName))
    {
        LE_KILL_CLIENT("Invalid app name.");
        return LE_NOT_FOUND;
    }

    LE_DEBUG("Received request to stop application '%s'.", appName);

    // Get the app object.
    AppContainer_t* appContainerPtr = GetActiveApp(appName);

    if (appContainerPtr == NULL)
    {
        LE_WARN("Application '%s' is not running and cannot be stopped.", appName);

        return LE_NOT_FOUND;
    }

    // Save this commands reference in this app.
    appContainerPtr->stopCmdRef = cmdRef;

    // Set the handler to be called when this app stops.  This handler will also respond to the
    // process that requested this app be stopped.
    appContainerPtr->stopHandler = stopHandler;

    // Stop the process.  This is an asynchronous call that returns right away.
    app_Stop(appContainerPtr->appRef);

    // If the application has already stopped then call its stop handler here.  Otherwise the stop
    // handler will be called from the AppStopHandler() when the app actually stops.
    if (app_GetState(appContainerPtr->appRef) == APP_STATE_STOPPED)
    {
        appContainerPtr->stopHandler(appContainerPtr);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to an application.
 *
 * @return
 *      Reference to the named app.
 *      NULL if the app is not installed.
 */
//--------------------------------------------------------------------------------------------------
void le_appCtrl_GetRef
(
    le_appCtrl_ServerCmdRef_t cmdRef,       ///< [IN] Command reference that must be passed to this
                                            ///       command's response function.
    const char* appName                     ///< [IN] Name of the app to get the ref for.
)
{
    le_appCtrl_GetRefRespond(cmdRef, appCtrl_GetRef(appName));
}


//--------------------------------------------------------------------------------------------------
/**
 * Release the reference to an application.
 */
//--------------------------------------------------------------------------------------------------
void le_appCtrl_ReleaseRef
(
    le_appCtrl_ServerCmdRef_t cmdRef,   ///< [IN] Command reference that must be passed to this
                                        ///       command's response function.
    le_appCtrl_AppRef_t appRef          ///< [IN] Ref to the app.
)
{
    appCtrl_ReleaseRef(appRef);

    le_appCtrl_ReleaseRefRespond(cmdRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the run flag for a process in an application.
 *
 * If there is an error this function will kill the calling client.
 */
//--------------------------------------------------------------------------------------------------
void le_appCtrl_SetRun
(
    le_appCtrl_ServerCmdRef_t cmdRef,   ///< [IN] Command reference that must be passed to this
                                        ///       command's response function.
    le_appCtrl_AppRef_t appRef,         ///< [IN] Ref to the app.
    const char* procName,               ///< [IN] Process name to set the run flag for.
    bool run                            ///< [IN] Flag to run the process or not.
)
{
    appCtrl_SetRun(appRef, procName, run);
    le_appCtrl_SetRunRespond(cmdRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the debug flag for a process in an application.
 *
 * If there is an error this function will kill the calling client.
 */
//--------------------------------------------------------------------------------------------------
void le_appCtrl_SetDebug
(
    le_appCtrl_ServerCmdRef_t cmdRef,   ///< [IN] Command reference that must be passed to this
                                        ///       command's response function.
    le_appCtrl_AppRef_t appRef,         ///< [IN] Ref to the app.
    const char* procName,               ///< [IN] Process name to set the debug flag for.
    bool debug                          ///< [IN] Flag to debug the process or not.
)
{
    appCtrl_SetDebug(appRef, procName, debug);
    le_appCtrl_SetDebugRespond(cmdRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Import a file into the app's working directory.
 *
 * @note
 *   The result code for this command should be sent back to the requesting process via
 *   le_appCtrl_ImportRespond().  The possible result codes are:
 *
 * @return
 *      LE_OK if successfully imported the file.
 *      LE_DUPLICATE if the path conflicts with items already in the app's working directory.
 *      LE_NOT_FOUND if the path does not point to a valid file.
 *      LE_BAD_PARAMETER if the path is formatted incorrectly.
 *      LE_FAULT if there was some other error.
 *
 * @note If the caller is passing an invalid reference to the application, it is a fatal error,
 *       the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_appCtrl_Import
(
    le_appCtrl_ServerCmdRef_t cmdRef,   ///< [IN] Command reference that must be passed to this
                                        ///       command's response function.
    le_appCtrl_AppRef_t appRef,         ///< [IN] Ref to the app.
    const char* path                    ///< [IN] Absolute path to the file to import.
)
{
    AppContainer_t* appContainerPtr = le_ref_Lookup(AppMap, appRef);

    if (appContainerPtr == NULL)
    {
        le_appCtrl_ImportRespond(cmdRef, LE_FAULT);
        LE_KILL_CLIENT("Invalid application reference.");
        return;
    }

    // Check that the path is valid.
    if ( (path == NULL) || (strlen(path) == 0) )
    {
        LE_ERROR("Import path cannot be empty.");
        le_appCtrl_ImportRespond(cmdRef, LE_BAD_PARAMETER);
        return;
    }
    else if (strlen(path) >= LIMIT_MAX_PATH_BYTES)
    {
        LE_ERROR("Import path '%s' is too long.", path);
        le_appCtrl_ImportRespond(cmdRef, LE_BAD_PARAMETER);
        return;
    }
    else if (strlen(path) >= LIMIT_MAX_PATH_BYTES)
    {
        LE_ERROR("Import path '%s' is too long.", path);
        le_appCtrl_ImportRespond(cmdRef, LE_BAD_PARAMETER);
        return;
    }

    le_appCtrl_ImportRespond(cmdRef, app_AddLink(appContainerPtr->appRef, path));
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets a device file's permissions.
 *
 * @note
 *   The result code for this command should be sent back to the requesting process via
 *   le_appCtrl_SetDevicePermRespond().  The possible result codes are:
 *
 * @return
 *      LE_OK if successfully set the device's permissions.
 *      LE_NOT_FOUND if the path does not point to a valid device.
 *      LE_BAD_PARAMETER if the path is formatted incorrectly.
 *      LE_FAULT if there was some other error.
 *
 * @note If the caller is passing an invalid reference to the application, it is a fatal error,
 *       the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_appCtrl_SetDevicePerm
(
    le_appCtrl_ServerCmdRef_t cmdRef,   ///< [IN] Command reference that must be passed to this
                                        ///       command's response function.
    le_appCtrl_AppRef_t appRef,         ///< [IN] Reference to the app.
    const char* path,                   ///< [IN] Absolute path to the device.
    const char* permissions             ///< [IN] Permission to apply to the device.
)
{
    AppContainer_t* appContainerPtr = le_ref_Lookup(AppMap, appRef);

    if (appContainerPtr == NULL)
    {
        le_appCtrl_SetDevicePermRespond(cmdRef, LE_FAULT);
        LE_KILL_CLIENT("Invalid application reference.");
        return;
    }

    // Check that the path is valid.
    if ( (path == NULL) || (strlen(path) == 0) )
    {
        LE_ERROR("Device path cannot be empty.");
        le_appCtrl_SetDevicePermRespond(cmdRef, LE_BAD_PARAMETER);
        return;
    }
    else if (strlen(path) >= LIMIT_MAX_PATH_BYTES)
    {
        LE_ERROR("Device path '%s' is too long.", path);
        le_appCtrl_SetDevicePermRespond(cmdRef, LE_BAD_PARAMETER);
        return;
    }

    // Check that the permissions are valid.
    if ( (strcmp(permissions, "r") != 0) &&
         (strcmp(permissions, "w") != 0) &&
         (strcmp(permissions, "rw") != 0) )
    {
        LE_ERROR("Invalid permissions string %s.", permissions);
        le_appCtrl_SetDevicePermRespond(cmdRef, LE_BAD_PARAMETER);
        return;
    }

    le_appCtrl_SetDevicePermRespond(cmdRef, app_SetDevPerm(appContainerPtr->appRef,
                                                           path,
                                                           permissions));
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_appCtrl_TraceAttach'
 *
 * Event that indicates the process that can be attached to in the application being traced.
 */
//--------------------------------------------------------------------------------------------------
le_appCtrl_TraceAttachHandlerRef_t le_appCtrl_AddTraceAttachHandler
(
    le_appCtrl_AppRef_t appRef,
        ///< [IN] Ref to the app.

    le_appCtrl_TraceAttachHandlerFunc_t attachToPidPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    AppContainer_t* appContainerPtr = le_ref_Lookup(AppMap, appRef);

    if (appContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application reference.");
        return NULL;
    }

    // Check if a handler is already registered for this app.
    if (appContainerPtr->traceAttachHandler != NULL)
    {
        LE_KILL_CLIENT("An attach handler for %s is already registered.",
                        app_GetName(appContainerPtr->appRef));
        return NULL;
    }

    // Store the client's handler and context pointer.
    appContainerPtr->traceAttachHandler = attachToPidPtr;
    appContainerPtr->traceAttachContextPtr = contextPtr;

    // Set our generic handler function in the app.
    app_SetBlockCallback(appContainerPtr->appRef, ProcBlockHandler, appRef);

    void* handlerSafeRef = le_ref_CreateRef(AppAttachHandlerMap, appContainerPtr);

    // Get a seperate safe reference for this app container that is used as the handler safe ref.
    return handlerSafeRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_appCtrl_TraceAttach'
 */
//--------------------------------------------------------------------------------------------------
void le_appCtrl_RemoveTraceAttachHandler
(
    le_appCtrl_TraceAttachHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    AppContainer_t* appContainerPtr = le_ref_Lookup(AppAttachHandlerMap, addHandlerRef);

    if (appContainerPtr != NULL)
    {
        le_ref_DeleteRef(AppAttachHandlerMap, addHandlerRef);

        app_SetBlockCallback(appContainerPtr->appRef, NULL, NULL);
        appContainerPtr->traceAttachHandler = NULL;
        appContainerPtr->traceAttachContextPtr = NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Unblocks the traced process.  This should normally be done once the tracer has successfully
 * attached to the process.
 *
 * @note If the caller is passing an invalid reference to the application, it is a fatal error,
 *       the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_appCtrl_TraceUnblock
(
    le_appCtrl_ServerCmdRef_t _cmdRef,
    le_appCtrl_AppRef_t appRef,
    int32_t pid
)
{
    AppContainer_t* appContainerPtr = le_ref_Lookup(AppMap, appRef);

    if (appContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application reference.");
        return;
    }

    app_Unblock(appContainerPtr->appRef, pid);

    le_appCtrl_TraceUnblockRespond(_cmdRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts an app.  This function is called by the event loop when a separate process requests to
 * start an app.
 *
 * @note
 *   The result code for this command should be sent back to the requesting process via
 *   le_appCtrl_StartRespond().  The possible result codes are:
 *
 *      LE_OK if the app is successfully started.
 *      LE_DUPLICATE if the app is already running.
 *      LE_NOT_FOUND if the app is not installed.
 *      LE_FAULT if there was an error and the app could not be launched.
 */
//--------------------------------------------------------------------------------------------------
void le_appCtrl_Start
(
    le_appCtrl_ServerCmdRef_t cmdRef,   ///< [IN] Command reference that must be passed to this
                                        ///       command's response function.
    const char* appName                 ///< [IN] Name of the application to start.
)
{
    le_result_t result = appCtrl_Start(appName);
    le_appCtrl_StartRespond(cmdRef, result);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops an app. This function is called by the event loop when a separate process requests to stop
 * an app.
 *
 * @note
 *   The result code for this command should be sent back to the requesting process via
 *   le_appCtrl_StopRespond(). The possible result codes are:
 *
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the app could not be found.
 */
//--------------------------------------------------------------------------------------------------
void le_appCtrl_Stop
(
    le_appCtrl_ServerCmdRef_t cmdRef,   ///< [IN] Command reference that must be passed to this
                                        ///       command's response function.
    const char* appName                 ///< [IN] Name of the application to stop.
)
{
    if (appCtrl_Stop(cmdRef, appName, RespondToStopAppCmd) != LE_OK)
    {
        le_appCtrl_StopRespond(cmdRef, LE_NOT_FOUND);
    }
}


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
    const char* appName
        ///< [IN]
        ///< Name of the application.
)
{
    if (!IsAppNameValid(appName))
    {
        LE_KILL_CLIENT("Invalid app name.");
        return LE_APPINFO_STOPPED;
    }

    // Search the list of apps.
    le_dls_Link_t* appLinkPtr = le_dls_Peek(&ActiveAppsList);

    while (appLinkPtr != NULL)
    {
        AppContainer_t* appContainerPtr = CONTAINER_OF(appLinkPtr, AppContainer_t, link);

        if (strncmp(app_GetName(appContainerPtr->appRef), appName, LIMIT_MAX_APP_NAME_BYTES) == 0)
        {
            switch (app_GetState(appContainerPtr->appRef))
            {
                case APP_STATE_STOPPED:
                    return LE_APPINFO_STOPPED;

                case APP_STATE_RUNNING:
                    return LE_APPINFO_RUNNING;

                default:
                    LE_FATAL("Unrecognized app state.");
            }
        }

        appLinkPtr = le_dls_PeekNext(&ActiveAppsList, appLinkPtr);
    }

    return LE_APPINFO_STOPPED;
}


//--------------------------------------------------------------------------------------------------
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
//--------------------------------------------------------------------------------------------------
le_appInfo_ProcState_t le_appInfo_GetProcState
(
    const char* appName,
        ///< [IN]
        ///< Name of the application.

    const char* procName
        ///< [IN]
        ///< Name of the process.
)
{
    if (!IsAppNameValid(appName))
    {
        LE_KILL_CLIENT("Invalid app name.");
        return LE_APPINFO_PROC_STOPPED;
    }

    if (!IsProcNameValid(procName))
    {
        LE_KILL_CLIENT("Invalid process name.");
        return LE_APPINFO_PROC_STOPPED;
    }

    // Search the list of apps.
    le_dls_Link_t* appLinkPtr = le_dls_Peek(&ActiveAppsList);

    while (appLinkPtr != NULL)
    {
        AppContainer_t* appContainerPtr = CONTAINER_OF(appLinkPtr, AppContainer_t, link);

        if (strncmp(app_GetName(appContainerPtr->appRef), appName, LIMIT_MAX_APP_NAME_BYTES) == 0)
        {
            switch (app_GetProcState(appContainerPtr->appRef, procName))
            {
                case APP_PROC_STATE_STOPPED:
                    return LE_APPINFO_PROC_STOPPED;

                case APP_PROC_STATE_RUNNING:
                    return LE_APPINFO_PROC_RUNNING;

                default:
                    LE_FATAL("Unrecognized proc state.");
            }
        }

        appLinkPtr = le_dls_PeekNext(&ActiveAppsList, appLinkPtr);
    }

    return LE_APPINFO_PROC_STOPPED;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the application name of the process with the specified PID.
 *
 * @return
 *      LE_OK if the application name was successfully found.
 *      LE_OVERFLOW if the application name could not fit in the provided buffer.
 *      LE_NOT_FOUND if the process is not part of an application.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_appInfo_GetName
(
    int32_t pid,
        ///< [IN]
        ///< PID of the process.

    char* appName,
        ///< [OUT]
        ///< Application name

    size_t appNameNumElements
        ///< [IN]
)
{
    char cgroupFilePath[LIMIT_MAX_PATH_BYTES] = {0};

    LE_ASSERT(snprintf(cgroupFilePath, sizeof(cgroupFilePath), "/proc/%d/cgroup", pid)
              < sizeof(cgroupFilePath));

    FILE* cgroupFilePtr = fopen(cgroupFilePath, "r");

    if (cgroupFilePtr == NULL)
    {
        LE_INFO("Cannot open %s. %m.", cgroupFilePath);
        return LE_FAULT;
    }

    // Other than the cgroup path which contains an app name, allocate another 20 bytes for
    // hierarchy ID, controller list, and misc. separators.
    char lineBuf[LIMIT_MAX_APP_NAME_LEN + 20] = {0};

    // Read the first line.
    LE_ASSERT(fgets(lineBuf, sizeof(lineBuf), cgroupFilePtr) != NULL);

    // Close the stream
    if (fclose(cgroupFilePtr) != 0)
    {
        if (errno == EINTR)
        {
            LE_WARN("Closing '%s' caused EINTR. Proceeding anyway.", cgroupFilePath);
        }
        else
        {
            LE_FATAL("Failed to close '%s'. Errno = %d (%m).", cgroupFilePath, errno);
        }
    }

    // Remove the trailing newline char.
    size_t len = strlen(lineBuf);

    if (lineBuf[len - 1] == '\n')
    {
        lineBuf[len - 1] = '\0';
    }

    // The line is expected to be in this format: "hierarchy-ID:controller-list:cgroup-path"
    // e.g. 4:freezer:/SomeApp
    // We are trying to get the 3rd token and remove the leading slash.
    char* token;
    char delim[2] = ":";

    strtok(lineBuf, delim);
    strtok(NULL, delim);
    token = strtok(NULL, delim);

    if (NULL == token)
    {
        LE_CRIT("Unexpected format for '%s'", lineBuf);
        return LE_FAULT;
    }

    // If the token has only one char (which is "/"), then the pid doesn't belong to any cgroup, and
    // hence not part of any app.
    if (strlen(token) <= 1)
    {
        return LE_NOT_FOUND;
    }

    // Note that the leading slash of the token has to be removed.
    return le_utf8_Copy(appName, (token + 1), appNameNumElements, NULL);
}


//--------------------------------------------------------------------------------------------------
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
//--------------------------------------------------------------------------------------------------
le_result_t le_appInfo_GetHash
(
    const char* appName,
        ///< [IN]
        ///< Application name.

    char* hashStr,
        ///< [OUT]
        ///< Hash string.

    size_t hashStrNumElements
        ///< [IN]
)
{
#define APP_INFO_FILE                       "info.properties"
#define KEY_STR_MD5                         "app.md5"

    if (!IsAppNameValid(appName))
    {
        LE_KILL_CLIENT("Invalid app name.");
        return LE_FAULT;
    }

    // Get the path to the app's info file.
    char infoFilePath[LIMIT_MAX_PATH_BYTES] = APPS_INSTALL_DIR;
    LE_ERROR_IF(le_path_Concat("/",
                               infoFilePath,
                               sizeof(infoFilePath),
                               appName,
                               APP_INFO_FILE,
                               NULL) != LE_OK,
                "Path to app %s's %s is too long.", appName, APP_INFO_FILE);

    // Check if the file exists.
    struct stat statBuf;

    if (stat(infoFilePath, &statBuf) == -1)
    {
        if (errno == ENOENT)
        {
            return LE_NOT_FOUND;
        }

        LE_ERROR("Could not stat file '%s'.  %m.", infoFilePath);
        return LE_FAULT;
    }

    // Get the md5 hash for the app's info.properties file.
    le_result_t result = properties_GetValueForKey(infoFilePath,
                                                   KEY_STR_MD5,
                                                   hashStr,
                                                   hashStrNumElements);

    switch(result)
    {
        case LE_OK:
        case LE_OVERFLOW:
            return result;

        default:
            return LE_FAULT;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * A watchdog has timed out. This function determines the watchdogAction to take and applies it.
 * The action to take is first delegated to the app (and proc layers) and actions not handled by
 * or not appropriate for lower layers are handled here.
 */
//--------------------------------------------------------------------------------------------------
void wdog_WatchdogTimedOut
(
    wdog_ServerCmdRef_t cmdRef,
    uint32_t procId
)
{
    wdog_WatchdogTimedOutRespond(cmdRef);
    LE_INFO("Handling watchdog expiry for: procId %d", procId);

    // Search for the process in the list of apps.
    le_dls_Link_t* appLinkPtr = le_dls_Peek(&ActiveAppsList);

    while (appLinkPtr != NULL)
    {
        AppContainer_t* appContainerPtr = CONTAINER_OF(appLinkPtr, AppContainer_t, link);

        wdog_action_WatchdogAction_t watchdogAction = WATCHDOG_ACTION_NOT_FOUND;

        LE_FATAL_IF(appContainerPtr == NULL, "Got a NULL AppPtr from CONTAINER_OF");

        if (app_WatchdogTimeoutHandler(appContainerPtr->appRef, procId, &watchdogAction) == LE_OK)
        {
            // Handle the fault.
            switch (watchdogAction)
            {
                case WATCHDOG_ACTION_NOT_FOUND:
                    // This case should already have been dealt with in lower layers, should never
                    // get here.
                    LE_FATAL("Unhandled watchdog action not found caught by supervisor.");

                case WATCHDOG_ACTION_IGNORE:
                case WATCHDOG_ACTION_HANDLED:
                    // Do nothing.
                    break;

                case WATCHDOG_ACTION_REBOOT:
                    ///< @todo Need to use a reboot API here that actually reboots the entire module
                    ///        rather than just the framework so that possibly connected peripherals
                    ///        get reset as well.  So, for now we will just log an error message and
                    ///        restart the app.
                    LE_EMERG("PID %d in app '%s' faulted: Rebooting system.",
                             procId,
                             app_GetName(appContainerPtr->appRef));
                    framework_Reboot();
                    break;

                case WATCHDOG_ACTION_RESTART_APP:
                    if (app_GetState(appContainerPtr->appRef) != APP_STATE_STOPPED)
                    {
                        // Stop the app if it hasn't already stopped.
                        app_Stop(appContainerPtr->appRef);
                    }

                    // Set the handler to restart the app when the app stops.
                    appContainerPtr->stopHandler = RestartApp;
                    break;

                case WATCHDOG_ACTION_STOP_APP:
                    if (app_GetState(appContainerPtr->appRef) != APP_STATE_STOPPED)
                    {
                        // Stop the app if it hasn't already stopped.
                        app_Stop(appContainerPtr->appRef);
                    }
                    break;

                // This should never happen
                case WATCHDOG_ACTION_ERROR:
                    LE_FATAL("Unhandled watchdog action error caught by supervisor.");

                // This should never happen
                default:
                    LE_FATAL("Unknown watchdog action %d.", watchdogAction);
            }

            // Check if the app has stopped.
            if ( (app_GetState(appContainerPtr->appRef) == APP_STATE_STOPPED) &&
                 (appContainerPtr->stopHandler != NULL) )
            {
                // The application has stopped.  Call the app stop handler.
                appContainerPtr->stopHandler(appContainerPtr);
            }

            // Stop searching the other apps.
            break;
        }

        appLinkPtr = le_dls_PeekNext(&ActiveAppsList, appLinkPtr);
    }

    if (appLinkPtr == NULL)
    {
        // We exhausted the app list without taking any action for this process
        LE_CRIT("Process pid:%d was not started by the framework. No watchdog action can be taken",
                procId);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a process in an app.  This function can be used to create and subsequently start a
 * process in an application that the application normally would not start on its own.  This
 * function does not actually start the process, use Start() to start the process.
 *
 * If the specified process name matches a name in the app's list of configured processes then
 * runtime parameters such as environment variables, priority, etc. will be taken from the
 * configuration database.  Otherwise default parameters will be used.
 *
 * Parameters can be overridden by the other functions in this API such as AddArg(), SetPriority(),
 * etc.
 *
 * If the executable path is empty and the process name matches a configured process then the
 * configured executable is used.  Otherwise the specified executable path is used.
 *
 * Either the process name or the executable path may be empty but not both.
 *
 * It is an error to call this function on a configured process that is already running.
 *
 * @return
 *      Reference to the application process object if successful.
 *      NULL if there was an error.
 *
 * @note If the application or process names pointers are null or if their strings are empty or of
 *       bad format it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_appProc_RefRef_t le_appProc_Create
(
    const char* appName,
        ///< [IN] Name of the app.

    const char* procName,
        ///< [IN] Name of the process.

    const char* execPath
        ///< [IN] Path to the executable file.
)
{
    // Check inputs.
    if (!IsAppNameValid(appName))
    {
        LE_KILL_CLIENT("Invalid app name.");
        return NULL;
    }

    // Ifgen does not allow NULL pointers to strings.  Translate empty strings to NULLs.
    const char* procNamePtr;
    if (strcmp(procName, "") == 0)
    {
        procNamePtr = NULL;
    }
    else
    {
        procNamePtr = procName;
    }

    const char* execPathPtr;
    if (strcmp(execPath, "") == 0)
    {
        execPathPtr = NULL;
    }
    else
    {
        execPathPtr = execPath;
    }

    if ( (procNamePtr == NULL) && (execPathPtr == NULL) )
    {
        LE_KILL_CLIENT("Process name and executable path cannot both be empty.");
        return NULL;
    }

    // Create the app if it doesn't already exist.
    AppContainer_t* appContainerPtr;
    le_result_t result = CreateApp(appName, &appContainerPtr);
    if (result != LE_OK)
    {
        return NULL;
    }

    // Create the app process for this app.
    app_Proc_Ref_t procRef = app_CreateProc(appContainerPtr->appRef, procNamePtr, execPathPtr);

    if (procRef == NULL)
    {
        return NULL;
    }

    // Check that we don't already have a reference to this process.
    if (IsAppProcAlreadyReferenced(procRef))
    {
        LE_KILL_CLIENT("Process is already referenced by a client.");
        return NULL;
    }

    // Create the app proc container to store stuff like the client session reference.
    AppProcContainer_t* appProcContainerPtr = le_mem_ForceAlloc(AppProcContainerPool);

    appProcContainerPtr->appContainerPtr = appContainerPtr;
    appProcContainerPtr->procRef = procRef;
    appProcContainerPtr->clientRef = le_appProc_GetClientSessionRef();

    // Get a safe reference for this app proc.
    return le_ref_CreateRef(AppProcMap, appProcContainerPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the file descriptor that the application process's standard in should be attached to.
 *
 * By default the standard in is directed to /dev/null.
 *
 * If there is an error this function will kill the calling process
 */
//--------------------------------------------------------------------------------------------------
void le_appProc_SetStdIn
(
    le_appProc_RefRef_t appProcRef,
        ///< [IN] Application process to start.

    int stdInFd
        ///< [IN] File descriptor to use as the app proc's standard in.
)
{
    AppProcContainer_t* appProcContainerPtr = le_ref_Lookup(AppProcMap, appProcRef);

    if (appProcContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application process reference.");
        return;
    }

    app_SetProcStdIn(appProcContainerPtr->procRef, stdInFd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the file descriptor that the application process's standard out should be attached to.
 *
 * By default the standard out is directed to the logs.
 *
 * If there is an error this function will kill the calling process
 */
//--------------------------------------------------------------------------------------------------
void le_appProc_SetStdOut
(
    le_appProc_RefRef_t appProcRef,
        ///< [IN] Application process to start.

    int stdOutFd
        ///< [IN] File descriptor to use as the app proc's standard out.
)
{
    AppProcContainer_t* appProcContainerPtr = le_ref_Lookup(AppProcMap, appProcRef);

    if (appProcContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application process reference.");
        return;
    }

    app_SetProcStdOut(appProcContainerPtr->procRef, stdOutFd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the file descriptor that the application process's standard err should be attached to.
 *
 * By default the standard err is directed to the logs.
 *
 * If there is an error this function will kill the calling process
 */
//--------------------------------------------------------------------------------------------------
void le_appProc_SetStdErr
(
    le_appProc_RefRef_t appProcRef,
        ///< [IN] Application process to start.

    int stdErrFd
        ///< [IN] File descriptor to use as the app proc's standard error.
)
{
    AppProcContainer_t* appProcContainerPtr = le_ref_Lookup(AppProcMap, appProcRef);

    if (appProcContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application process reference.");
        return;
    }

    app_SetProcStdErr(appProcContainerPtr->procRef, stdErrFd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_appProc_Stop'
 *
 * Process stopped event.
 */
//--------------------------------------------------------------------------------------------------
le_appProc_StopHandlerRef_t le_appProc_AddStopHandler
(
    le_appProc_RefRef_t appProcRef,
        ///< [IN] Application process to start.

    le_appProc_StopHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    AppProcContainer_t* appProcContainerPtr = le_ref_Lookup(AppProcMap, appProcRef);

    if (appProcContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application process reference.");
        return NULL;
    }

    app_SetProcStopHandler(appProcContainerPtr->procRef, handlerPtr, contextPtr);

    // There is only one handler for each proc so just return the appProcRef which can be used to
    // find the handler.
    return (le_appProc_StopHandlerRef_t)appProcRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_appProc_Stop'
 */
//--------------------------------------------------------------------------------------------------
void le_appProc_RemoveStopHandler
(
    le_appProc_StopHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    AppProcContainer_t* appProcContainerPtr = le_ref_Lookup(AppProcMap, addHandlerRef);

    if (appProcContainerPtr == NULL)
    {
        // Client may have already been deleted.
        return;
    }

    // Clear the handler.
    app_SetProcStopHandler(appProcContainerPtr->procRef, NULL, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a command line argument to the application process.
 *
 * If the application process is a configured process adding any argument means no arguments from
 * the configuration database will be used.
 *
 * Adding an empty argument validates the argument list but does not acutally add an argument.  This
 * is useful for overriding the configured arguments list with an empty argument list.
 *
 * If there is an error this function will kill the calling client.
 */
//--------------------------------------------------------------------------------------------------
void le_appProc_AddArg
(
    le_appProc_RefRef_t appProcRef,
        ///< [IN] Application process to start.

    const char* arg
        ///< [IN] Argument to add.
)
{
    AppProcContainer_t* appProcContainerPtr = le_ref_Lookup(AppProcMap, appProcRef);

    if (appProcContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application process reference.");
        return;
    }

    if (app_AddArgs(appProcContainerPtr->procRef, arg) != LE_OK)
    {
        LE_KILL_CLIENT("Argument '%s' is too long.", arg);
        return;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes and invalidates the cmd-line arguments to a process.  This means the process will only
 * use arguments from the config if available.
 *
 * @note If the caller is passing an invalid reference to the application process, it is a fatal
 *       error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_appProc_ClearArgs
(
    le_appProc_RefRef_t appProcRef
        ///< [IN] Application process to start.
)
{
    AppProcContainer_t* appProcContainerPtr = le_ref_Lookup(AppProcMap, appProcRef);

    if (appProcContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application process reference.");
        return;
    }

    app_ClearArgs(appProcContainerPtr->procRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the application process's priority.
 *
 * The priority string must be either 'idle','low', 'medium', 'high', 'rt1', 'rt2'...'rt32'.
 *
 * If there is an error this function will kill the calling client.
 */
//--------------------------------------------------------------------------------------------------
void le_appProc_SetPriority
(
    le_appProc_RefRef_t appProcRef,
        ///< [IN] Application process to start.

    const char* priority
        ///< [IN] Priority for the application process.
)
{
    AppProcContainer_t* appProcContainerPtr = le_ref_Lookup(AppProcMap, appProcRef);

    if (appProcContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application process reference.");
        return;
    }

    le_result_t result = app_SetProcPriority(appProcContainerPtr->procRef, priority);

    if (result == LE_OVERFLOW)
    {
        LE_KILL_CLIENT("Priority string '%s' is too long.", priority);
        return;
    }

    if (result == LE_FAULT)
    {
        LE_KILL_CLIENT("Priority string '%s' is invalid.", priority);
        return;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Clears the application process's priority and use either the configured priority or the default.
 *
 * @note If the caller is passing an invalid reference to the application process, it is a fatal
 *       error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_appProc_ClearPriority
(
    le_appProc_RefRef_t appProcRef
        ///< [IN] Application process to start.
)
{
    AppProcContainer_t* appProcContainerPtr = le_ref_Lookup(AppProcMap, appProcRef);

    if (appProcContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application process reference.");
        return;
    }

    LE_ASSERT(app_SetProcPriority(appProcContainerPtr->procRef, NULL) == LE_OK);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the application process's fault action.
 *
 * @note If the caller is passing an invalid reference to the application process, it is a fatal
 *       error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_appProc_SetFaultAction
(
    le_appProc_RefRef_t appProcRef,
        ///< [IN] Application process to start.

    le_appProc_FaultAction_t action
        ///< [IN] Fault action for the application process.
)
{
    AppProcContainer_t* appProcContainerPtr = le_ref_Lookup(AppProcMap, appProcRef);

    if (appProcContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application process reference.");
        return;
    }

    FaultAction_t faultAction = FAULT_ACTION_NONE;

    switch (action)
    {
        case LE_APPPROC_FAULT_ACTION_IGNORE:
            faultAction = FAULT_ACTION_IGNORE;
            break;

        case LE_APPPROC_FAULT_ACTION_RESTART_PROC:
            faultAction = FAULT_ACTION_RESTART_PROC;
            break;

        case LE_APPPROC_FAULT_ACTION_RESTART_APP:
            faultAction = FAULT_ACTION_RESTART_APP;
            break;

        case LE_APPPROC_FAULT_ACTION_STOP_APP:
            faultAction = FAULT_ACTION_STOP_APP;
            break;

        case LE_APPPROC_FAULT_ACTION_REBOOT:
            faultAction = FAULT_ACTION_REBOOT;
            break;

        default:
            LE_KILL_CLIENT("Invalid fault action.");
            return;
    }

    app_SetFaultAction(appProcContainerPtr->procRef, faultAction);
}


//--------------------------------------------------------------------------------------------------
/**
 * Clears the application process's fault action and use either the configured fault action or the
 * default.
 *
 * @note If the caller is passing an invalid reference to the application process, it is a fatal
 *       error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_appProc_ClearFaultAction
(
    le_appProc_RefRef_t appProcRef
        ///< [IN] Application process to start.
)
{
    AppProcContainer_t* appProcContainerPtr = le_ref_Lookup(AppProcMap, appProcRef);

    if (appProcContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application process reference.");
        return;
    }

    app_SetFaultAction(appProcContainerPtr->procRef, FAULT_ACTION_NONE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the debug flag for a process.
 */
//--------------------------------------------------------------------------------------------------
void le_appProc_SetDebug
(
    le_appProc_RefRef_t appProcRef,   ///< [IN] Application process to debug
    bool debug                        ///< [IN] Debug flag
)
{
    AppProcContainer_t* appProcContainerPtr = le_ref_Lookup(AppProcMap, appProcRef);

    if (appProcContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application process reference.");
        return;
    }

    app_SetDebug(appProcContainerPtr->procRef, debug);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts the application process.  If the application was not running this function will start it
 * first.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was some other error.
 *
 * @note If the caller is passing an invalid reference to the application process, it is a fatal
 *       error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_appProc_Start
(
    le_appProc_RefRef_t appProcRef
        ///< [IN] Application process to start.
)
{
    AppProcContainer_t* appProcContainerPtr = le_ref_Lookup(AppProcMap, appProcRef);

    if (appProcContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application process reference.");
        return LE_FAULT;
    }

    // Start the app if it isn't already running.
    if (app_GetState(appProcContainerPtr->appContainerPtr->appRef) != APP_STATE_RUNNING)
    {
        le_result_t result = StartApp(appProcContainerPtr->appContainerPtr);

        if (result != LE_OK)
        {
            return LE_FAULT;
        }
    }

    // Start the process.
    return app_StartProc(appProcContainerPtr->procRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the application process object.
 *
 * @note If the caller is passing an invalid reference to the application process, it is a fatal
 *       error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_appProc_Delete
(
    le_appProc_RefRef_t appProcRef
        ///< [IN] Application process to start.
)
{
    AppProcContainer_t* appProcContainerPtr = le_ref_Lookup(AppProcMap, appProcRef);

    if (appProcContainerPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid application process reference.");
        return;
    }

    // Remove the safe reference.
    le_ref_DeleteRef(AppProcMap, appProcRef);

    app_DeleteProc(appProcContainerPtr->appContainerPtr->appRef, appProcContainerPtr->procRef);

    le_mem_Release(appProcContainerPtr);
}


// ---------------- Deprecated Functions -----------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Logs a deprecated API message.
 */
//--------------------------------------------------------------------------------------------------
static void LogDeprecatedMsg
(
    void
)
{
    LE_WARN("le_sup_ctrl.api is deprecated.  Please use le_appCtrl.api instead.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to an application.
 *
 * @return
 *      Reference to the named app.
 *      NULL if the app is not installed.
 */
//--------------------------------------------------------------------------------------------------
void le_sup_ctrl_GetAppRef
(
    le_sup_ctrl_ServerCmdRef_t _cmdRef,
    const char* appName
)
{
    LogDeprecatedMsg();
    le_sup_ctrl_GetAppRefRespond(_cmdRef, appCtrl_GetRef(appName));
}


//--------------------------------------------------------------------------------------------------
/**
 * Release the reference to an application.
 */
//--------------------------------------------------------------------------------------------------
void le_sup_ctrl_ReleaseAppRef
(
    le_sup_ctrl_ServerCmdRef_t _cmdRef,
    le_sup_ctrl_AppRef_t appRef
)
{
    LogDeprecatedMsg();
    appCtrl_ReleaseRef((le_appCtrl_AppRef_t)appRef);
    le_sup_ctrl_ReleaseAppRefRespond(_cmdRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the run flag for a process in an application.
 *
 * If there is an error this function will kill the calling client.
 */
//--------------------------------------------------------------------------------------------------
void le_sup_ctrl_SetRun
(
    le_sup_ctrl_ServerCmdRef_t _cmdRef,
    le_sup_ctrl_AppRef_t appRef,
    const char* procName,
    bool run
)
{
    LogDeprecatedMsg();
    appCtrl_SetRun((le_appCtrl_AppRef_t)appRef, procName, run);
    le_sup_ctrl_SetRunRespond(_cmdRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts an app.  This function is called by the event loop when a separate process requests to
 * start an app.
 *
 * @note
 *   The result code for this command should be sent back to the requesting process via
 *   le_sup_ctrl_StartAppRespond().  The possible result codes are:
 *
 *      LE_OK if the app is successfully started.
 *      LE_DUPLICATE if the app is already running.
 *      LE_NOT_FOUND if the app is not installed.
 *      LE_FAULT if there was an error and the app could not be launched.
 */
//--------------------------------------------------------------------------------------------------
void le_sup_ctrl_StartApp
(
    le_sup_ctrl_ServerCmdRef_t _cmdRef,
    const char* appName
)
{
    LogDeprecatedMsg();
    le_sup_ctrl_StartAppRespond(_cmdRef, appCtrl_Start(appName));
}


//--------------------------------------------------------------------------------------------------
/**
 * Responds to the stop app command.  Also deactivates the app container for the app that just
 * stopped.
 */
//--------------------------------------------------------------------------------------------------
static void RespondToStopAppCmdDeprecated
(
    AppContainer_t* appContainerPtr           ///< [IN] App that stopped.
)
{
    // Save command reference for later use.
    void* cmdRef = appContainerPtr->stopCmdRef;

    DeactivateAppContainer(appContainerPtr);

    // Respond to the requesting process.
    le_sup_ctrl_StopAppRespond(cmdRef, LE_OK);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops an app. This function is called by the event loop when a separate process requests to stop
 * an app.
 *
 * @note
 *   The result code for this command should be sent back to the requesting process via
 *   le_sup_ctrl_StopAppRespond(). The possible result codes are:
 *
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the app could not be found.
 */
//--------------------------------------------------------------------------------------------------
void le_sup_ctrl_StopApp
(
    le_sup_ctrl_ServerCmdRef_t _cmdRef,
    const char* appName
)
{
    LogDeprecatedMsg();

    if (appCtrl_Stop((le_appCtrl_ServerCmdRef_t)_cmdRef, appName,
                      RespondToStopAppCmdDeprecated) != LE_OK)
    {
        le_sup_ctrl_StopAppRespond(_cmdRef, LE_NOT_FOUND);
    }
}
