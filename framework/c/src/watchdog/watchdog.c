//--------------------------------------------------------------------------------------------------
/**
 * @file watchdog.c
 *
 * @section wd_intro        Introduction
 *
 * The watchdog service provides a means of recovering the system if apps or components cease
 * functioning due to infinite loops, deadlocks and waiting on things that never happen.
 * By having a process call the le_wdog_Kick() method that process now becomes watched and if
 * le_wdog_Kick() is not called again within the configured time the process will, depending on the
 * configured action, be stopped, terminated or restarted, or the entire app may be restarted.
 */
//--------------------------------------------------------------------------------------------------
 /** @section wd_tedious_detail            More involved discussion follows
 *
 * The watchdog runs as a service which mimics a hardware watchdog to a certain extent except that
 * a) It isn't hardware.
 * b) It can initiate corrective actions other than taking the entire system down.
 * c) It can offer this service independently to several processes and apps and act only on those
 *    apps when a fault arises.
 *
 * Apps should configure a default time out and watchdog fault action before they make use of the
 * watchdog. If a timeout is not configured a warning will be issued in the logs on the first use
 * of the le_wdog and a timeout of 30 seconds will be used. The following sections in the adef file
 * control watchdog behaviour.
 *
 *      watchdogTimeout: <number of millisecond>
 *      // TODO: watchdogAction is not yet implemented and will be handled by the supervisor
 *      watchdogAction:
 *
 *
 *
 * Algorithm
 * When a process kicks us, if we have no timer for it we will:
 *    create a timer,
 *    add it to our timer list and
 *    set it running with the appropriate time out (for now, that configured for the app).
 * If the timer times out before the next kick then the watchdog will
 *    attempt to alert the supervisor that the app has timed out.
 *          The supervisor can then apply the configured fault action.
 *    delist the timer and dispose of it.
 *
 * Analysis
 *
 * case 1: A timeout received for a process that no longer exists (died by other
 *         means) will notify the the supervisor who will find it to be already dead so
 *         no action will be taken.
 * case 2: A kick may be received from a process that has just died (race condition) but
 *         the dead process won't be around to kick the watchdog again at which time
 *         we have case 1.
 * case 3: Another race condition - the app times out and we tell the supervisor about it.
 *         We delist the timer and destroy it.
 *         The supervisor kills the app but between the timeout and the supervisor acting
 *         the app sends a kick.
 *         We treat the kick as a kick from a new app and create a timer.
 *         When the timer times out we have case 1 again.
 *
 *         The analysis assumes that the time between timeouts is significantly shorter
 *         than the time expected before pIDs are re-used.
 *
 *
 *
 * Besides le_wdog_Kick(), a command to temporarily change the timeout is provided.
 * le_wdog_Timeout(milliseconds) will adjust the current timeout and restart the timer.
 * This timeout will be effective for one time only reverting to the default value at the next
 * le_wdog_Kick().
 *
 * There are two special timeout values, LE_WDOG_TIMEOUT_NOW and LE_WDOG_TIMEOUT_NEVER.
 *
 * LE_WDOG_TIMEOUT_NEVER will cause a timer to never time out. The largest attainable timeout value
 * that does time out is (LE_WDOG_TIMEOUT_NEVER - 1) which gives a timeout of about 49 days. If 49
 * days is not long enough for your purposes then LE_WDOG_TIMEOUT_NEVER will make sure that the
 * process can live indefinately without calling le_wdog_Kick(). If you find yourself using this
 * special value often you might want to reconsider whether you really want to use a watchdog timer
 * for your process.
 *
 * LE_WDOG_TIMEOUT_NOW could be used in development to see how the app responds to a timeout
 * situation though it could also be abused as a way to restart the app for some reason.
 *
 * If a watchdog was set to never time out and the process that created it ends without changing the
 * timeout value, either by le_wdog_Kick() or le_wdog_Timeout() then the wdog will not be freed. To
 * prevent a pileup of dead dogs the system periodically searches for watchdogs whose processes have
 * gone away and then frees them. The search is triggered when the number of watchdog objects
 * crosses an arbitrary threshhold. If all watchdogs are found to be owned by extant processes then
 * the threshold value is increased until a point at which all allowable watchdog resources have
 * been allocated at which point no more will be be created.
 *
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "../limit.h"
#include "interfaces.h"
#include "../user.h"
#include "../fileDescriptor.h"



//--------------------------------------------------------------------------------------------------
/**
 * Size of the watchdog hash table. Should be tuned to a prime number near to the expected
 * number of users of the watchdog service.
 **/
//--------------------------------------------------------------------------------------------------
#define LE_WDOG_HASTABLE_WIDTH  31 ///< Size of the Watchdog hash table

//--------------------------------------------------------------------------------------------------
/**
 * If this value is returned by le_cfg when trying to get the watchdog timeout then there is no
 * timeout configured. Use TIMEOUT_DEFAULT.
 **/
//--------------------------------------------------------------------------------------------------
#define CFG_TIMEOUT_USE_DEFAULT -2

//--------------------------------------------------------------------------------------------------
/**
 * The default timeout to use if no timeout is configured (in milliseconds)
 **/
//--------------------------------------------------------------------------------------------------
#define TIMEOUT_DEFAULT 30000

//--------------------------------------------------------------------------------------------------
/**
 * Use the watchdog timer's default kick timeout interval.
 **/
//--------------------------------------------------------------------------------------------------
#define TIMEOUT_KICK -3

//--------------------------------------------------------------------------------------------------
/**
 *  Definition of Watchdog object, pool for allocation of watchdogs and container for organizing and
 *  finding watchdog objects.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pid_t procId;                       ///< The unique value by which to find this watchdog
    uid_t appId;                        ///< The id of the app it belongs to
    le_clk_Time_t kickTimeoutInterval;  ///< Default timeout for this watchdog
    le_timer_Ref_t timer;               ///< The timer this watchdog uses
}
WatchdogObj_t;

static le_mem_PoolRef_t WatchdogPool;           ///< The memory pool the watchdogs will come from
static le_hashmap_Ref_t WatchdogRefsContainer;  ///< The container we use to keep track of wdogs

//--------------------------------------------------------------------------------------------------
/**
 * Remove the watchdog from our container, free the timer it contains and then free the storage
 * we allocated to hold the watchdog structure.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteWatchdog
(
    pid_t dogOwner  ///< The client (hash key) of the Watchdog that we want to dispose of
)
{
    WatchdogObj_t* deadDogPtr = le_hashmap_Remove(WatchdogRefsContainer, &dogOwner);
    if (deadDogPtr != NULL)
    {
        // All good. The dog was in the hash
        LE_DEBUG("Cleaning up watchdog resources for %d", deadDogPtr->procId);
        le_timer_Delete(deadDogPtr->timer);
        le_mem_Release(deadDogPtr);
    }
    else
    {
        // else the dog MUST already be deleted.
        LE_DEBUG("Cleaning up watchdog resources for %d but already freed.", dogOwner);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Put the watchdog into the container so we can use container methods to look it up again
 * when we need it.
 */
//--------------------------------------------------------------------------------------------------
static void AddWatchdog
(
    WatchdogObj_t* newDogPtr   ///< A pointer to the watchdog that is to be added to our container
)
{
    // The procId is the unique identifier for this watchdog. There shouldn't already be one.
    LE_ASSERT(NULL == le_hashmap_Put(WatchdogRefsContainer, &(newDogPtr->procId), newDogPtr));
}

//--------------------------------------------------------------------------------------------------
/**
 * When a client connection closes try to find any unexpired timers (or any other currently
 * unreleased resources) used by that session and release them. Timers may have already been
 * released if they have expired.
 */
//--------------------------------------------------------------------------------------------------
static void CleanUpClosedClient
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    uid_t clientUserId;
    pid_t clientProcId;

    LE_INFO("Client session closed");
    if (LE_OK == le_msg_GetClientUserCreds(sessionRef, &clientUserId, &clientProcId))
    {
        DeleteWatchdog(clientProcId);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Find the watchdog associated with this ID
 *
 *   @return A pointer to the watchdog associated with this client
 */
//--------------------------------------------------------------------------------------------------
static WatchdogObj_t* LookupClientWatchdogPtrById
(
    pid_t clientPid  ///< Client we want the watchdog for
)
{
    return le_hashmap_Get(WatchdogRefsContainer, &clientPid);
}

//--------------------------------------------------------------------------------------------------
/**
 * The handler for all time outs. No registered application wants to see us get here.
 * Arrival here means that some process has failed to service its watchdog and therefore,
 * we need to tattle to the supervisor who, if the app still exists, will deal with it
 * in the manner proscribed in the book of config.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WatchdogHandleExpiry
(
    le_timer_Ref_t timerRef ///< [IN] The reference to the expired timer
)
{
    char appName[LIMIT_MAX_APP_NAME_BYTES];
    pid_t procId = (intptr_t)le_timer_GetContextPtr(timerRef);
    WatchdogObj_t* expiredDog = LookupClientWatchdogPtrById(procId);
    if (expiredDog != NULL)
    {
        uid_t appId = expiredDog->appId;


        if (LE_OK == user_GetAppName(appId, appName, sizeof(appName) ))
        {
            LE_CRIT("app %s, proc %d timed out", appName, procId);
        }
        else
        {
            LE_CRIT("app %d, proc %d timed out", appId, procId);
        }

        DeleteWatchdog(procId);
        wdog_WatchdogTimedOut(appId, procId);
    }
    else
    {
        LE_CRIT("Processing watchdog timeout for proc %d but watchdog already freed.", procId);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Construct le_clk_Time_t object that will give an interval of the provided number
 *  of milliseconds.
 *
 *      @return the constructed le_clk_Time_t
 */
//--------------------------------------------------------------------------------------------------
static le_clk_Time_t MakeTimerInterval
(
    uint32_t milliseconds
)
{
    le_clk_Time_t interval;

    interval.sec = milliseconds / 1000;
    interval.usec = (milliseconds * 1000) - (interval.sec * 1000000);

    return interval;
}

//--------------------------------------------------------------------------------------------------
/**
 * Given the pid, find out what the process name is. The process name, if found, is written to
 * the supplied char* buffer "name" up to a number of characters given by size_t length.
 *
 *      @return LE_NOT_FOUND if no info could be retrieved for the pid
 *              LE_FAULT if the buffer pointer is NULL or if the reading of the pid info fails
 *              LE_OVERFLOW if the process info doesn't fit in the buffer
 *              LE_OK if the process name copied to the buffer is valid and can be safely used.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetProcessNameFromPid
(
    pid_t pId,      ///< [IN] The pid of the process whose name to find
    char* name,     ///< [OUT] A buffer to receive the name of the app
    size_t length   ///< [IN] The size of the buffer that receives the name
)
{
    char pathStr[LIMIT_MAX_PATH_BYTES] = "";
    char procPathStr[LIMIT_MAX_PATH_BYTES] = "";

    if (name != NULL)
    {

        // on linux, /proc/[pid]/cmdline contains the command and arguments separated by '\0's
        int result = snprintf(pathStr, sizeof(pathStr), "/proc/%d/cmdline", pId);
        if (result < 0 || result >= LIMIT_MAX_PATH_BYTES)
        {
            return LE_NOT_FOUND;
        }

        int fd = open(pathStr, O_RDONLY);
        if (fd)
        {
            result = read (fd, procPathStr, LIMIT_MAX_PATH_BYTES);
            fd_Close(fd);
            if (result == 0)
            {
                return LE_FAULT;
            }
            else if (strnlen(procPathStr, LIMIT_MAX_PATH_BYTES) ==  LIMIT_MAX_PATH_BYTES)
            {
                // We need the first parameter of the command line, which is path to a process.
                // This shouldn't be longer than LIMIT_MAX_PATH_BYTES.
                return LE_OVERFLOW;
            }
            // strip the path
            char* procNamePtr = le_path_GetBasenamePtr(procPathStr, "/");

            return le_utf8_Copy(name, procNamePtr, length, NULL);
        }
    }
    else
    {
        return LE_FAULT;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the configured timeout value for watchdogs belonging to this client process or, if that
 * is not found, read the configured timeout for the application this process belongs to.
 *
 * @return
 *      A le_clk_Time_t struct representing the configured timeout interval
 */
//--------------------------------------------------------------------------------------------------
static le_clk_Time_t GetConfigKickTimeoutInterval
(
    pid_t procId,  ///< The process id of the client
    uid_t appId    ///< The user id of the application
)
{
    char appName[LIMIT_MAX_APP_NAME_BYTES] = "";
    char procName[LIMIT_MAX_PROCESS_NAME_BYTES] = "";
    char configPath[LIMIT_MAX_PATH_BYTES] = "";

    const int defaultTimeout = TIMEOUT_DEFAULT;
    int proc_milliseconds = CFG_TIMEOUT_USE_DEFAULT;
    int app_milliseconds = CFG_TIMEOUT_USE_DEFAULT;

    if (LE_OK == user_GetAppName(appId, appName, sizeof(appName) ))
    {    // Check if there is a config for the process name first else check under the app name

        // It's a real app. Let's look up the config!
        LE_DEBUG("Getting configured watchdog timeout for app %s", appName);
        if (le_path_Concat("/", configPath, sizeof(configPath), "apps", appName,
                "watchdogTimeout", NULL) == LE_OK)
        {
            app_milliseconds = le_cfg_QuickGetInt(configPath, CFG_TIMEOUT_USE_DEFAULT);
        }

        if (LE_OK == GetProcessNameFromPid( procId, procName, sizeof(procName)))
        {
            // get the config
            configPath[0]='\0';
            LE_DEBUG("Getting configured watchdog timeout for process %s", procName);

            if(le_path_Concat("/", configPath, sizeof(configPath), "apps", appName, "procs",
                    procName, "watchdogTimeout", NULL) == LE_OK)
            {
                proc_milliseconds = le_cfg_QuickGetInt(configPath, CFG_TIMEOUT_USE_DEFAULT);
            }
        }

        // find a valid value starting at proc level and working up
        if (proc_milliseconds == CFG_TIMEOUT_USE_DEFAULT)
        {
            if (app_milliseconds == CFG_TIMEOUT_USE_DEFAULT)
            {
                proc_milliseconds = defaultTimeout;
                LE_WARN("No watchdog timeout configured for %s - using default %d ms", appName,
                  proc_milliseconds);
            }
            else
            {
                proc_milliseconds = app_milliseconds;
                LE_INFO("No watchdog timeout configured for process %s - using app timeout %d ms",
                    procName, proc_milliseconds);
            }
        }
        else
        {
            LE_DEBUG("Watchdog timeout configured for %s - timeout %d ms", procName,
              proc_milliseconds);
        }
    }
    else
    {
        // We have no idea what process is calling us, but we can set a default timeout
        // and play along.
        // TODO: Find a way to get the configured watchdog timeout duration for unsandboxed
        //       apps, which run as root.
        proc_milliseconds = defaultTimeout;
        LE_WARN("Unknown app with uid %u requested watchdog - using default timeout %d ms", appId,
          proc_milliseconds);
    }
    return MakeTimerInterval(proc_milliseconds);
}

//--------------------------------------------------------------------------------------------------
/**
 * Allocate a new watchdog object and "construct" it.
 *
 * @return
 *      A pointer to a new Watchdog object containing an initialized timer
 */
//--------------------------------------------------------------------------------------------------
static WatchdogObj_t* CreateNewWatchdog
(
    pid_t clientPid,   ///< The process id of the client
    uid_t appId       ///< the user id of the client
)
{
    char timerName[LIMIT_MAX_TIMER_NAME_BYTES];

    LE_DEBUG("Making a new dog");
    WatchdogObj_t* newDogPtr = le_mem_ForceAlloc(WatchdogPool);
    newDogPtr->procId = clientPid;
    newDogPtr->appId = appId;
    newDogPtr->kickTimeoutInterval = GetConfigKickTimeoutInterval(clientPid, appId);
    LE_ASSERT(0 <= snprintf(timerName, sizeof(timerName), "wdog_u%d:p%d", clientPid, appId));
    newDogPtr->timer = le_timer_Create(timerName);
    _Static_assert (sizeof(pid_t) <= sizeof(intptr_t), "pid_t is truncated by cast to void*");
    LE_ASSERT(LE_OK == le_timer_SetContextPtr(newDogPtr->timer, (void*)((intptr_t)clientPid)));
    LE_ASSERT(LE_OK == le_timer_SetHandler(newDogPtr->timer, WatchdogHandleExpiry));
    return newDogPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns the timer associated with the client requesting the service.
 * If no timer exists then one is created and associated with the client.
 *
 * @return The pointer to the watchdog associated with the client or a new one if none exists.
 *         May return a empty reference if the client has closed already.
 */
//--------------------------------------------------------------------------------------------------
static WatchdogObj_t* GetClientWatchdogPtr
(
    void
)
{
    /* Get the user id of the client */
    uid_t clientUserId;
    pid_t clientProcId;
    le_msg_SessionRef_t sessionRef = le_wdog_GetClientSessionRef();
    WatchdogObj_t* watchdogPtr = NULL;

    if (LE_OK == le_msg_GetClientUserCreds(sessionRef, &clientUserId, &clientProcId))
    {
        watchdogPtr = LookupClientWatchdogPtrById(clientProcId);
        if (watchdogPtr == NULL)
        {
            watchdogPtr = CreateNewWatchdog(clientProcId, clientUserId);
            AddWatchdog(watchdogPtr);
        }
    }
    else
    {
        LE_WARN("Can't find client Id. The client may have closed the session.");
    }
    return watchdogPtr;
}

//--------------------------------------------------------------------------------------------------
/**
* Resets the watchdog for the client that has kicked us. This function must be called from within
* the watchdog IPC events such as le_wdog_Timeout(), le_wdog_Kick().
**/
//--------------------------------------------------------------------------------------------------
static void ResetClientWatchdog
(
    int32_t timeout ///< [IN] The timeout to reset the watchdog timer to (in milliseconds).
)
{
    le_clk_Time_t timeoutValue;
    WatchdogObj_t* watchDogPtr = GetClientWatchdogPtr();
    if (watchDogPtr != NULL)
    {
        le_timer_Stop(watchDogPtr->timer);
        if (timeout == TIMEOUT_KICK)
        {
            timeoutValue = watchDogPtr->kickTimeoutInterval;
        }
        else
        {
            timeoutValue = MakeTimerInterval(timeout);
        }

        if (timeout != LE_WDOG_TIMEOUT_NEVER)
        {
            // timer should be stopped here so this should never fail
            // testing
            LE_ASSERT(LE_OK == le_timer_SetInterval(watchDogPtr->timer, timeoutValue));
            le_timer_Start(watchDogPtr->timer);
        }
        else
        {
            LE_DEBUG("Timeout set to NEVER!");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Adjust the timeout. This can be used if you need a different interval for the timeout on a
 * specific occassion. The new value of the timeout lasts until expiry or the next kick. On
 * the next kick, the timeout will revert to the original configured value.
 *
 * LE_WDOG_TIMEOUT_NEVER disables the watchdog (until it is kicked again or a new timeout is set)
 * LE_WDOG_TIMEOUT_NOW is a zero length interval and causes the watchdog to expire immediately.
 */
//--------------------------------------------------------------------------------------------------
void le_wdog_Timeout
(
    int32_t milliseconds ///< [IN] number of milliseconds before the watchdog expires.
)
{
    LE_DEBUG("Attempting to set new watchdog timeout to %d", milliseconds);
    ResetClientWatchdog(milliseconds);
}

//--------------------------------------------------------------------------------------------------
/**
 * Calling watchdog kick resets the watchdog expiration timer and briefly cheats death.
 */
//--------------------------------------------------------------------------------------------------
void le_wdog_Kick
(
    void
)
{
    LE_DEBUG("Attempting to kick the dog timer!");
    ResetClientWatchdog(TIMEOUT_KICK);
}

//--------------------------------------------------------------------------------------------------
/**
 * Signal to the supervisor that we are set up and ready
 */
//--------------------------------------------------------------------------------------------------
static void SystemProcessNotifySupervisor
(
    void
)
{
    // Close the fd that we inherited from the Supervisor.  This will let the Supervisor know that
    // we are initialized.  Then re-open it to /dev/null so that it cannot be reused later.
    FILE* filePtr;
    do
    {
        filePtr = freopen("/dev/null", "r", stdin);
    }
    while ( (filePtr == NULL) && (errno == EINTR) );

    LE_FATAL_IF(filePtr == NULL, "Failed to redirect standard in to /dev/null.  %m.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Create the memory pool to allocate watchdog objects from and the container to store them in
 * so we can find the ones we want when we want them. Currently that's a hashmap.
 *
 * @return
 *      LE_OK the timer container was successfully initiated
 *      Currently no other values can be returned. This function asserts if container can't be
 *      initiated.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t InitializeTimerContainer
(
    void
)
{
    WatchdogPool = le_mem_CreatePool("WatchdogPool", sizeof(WatchdogObj_t));
    WatchdogRefsContainer = le_hashmap_Create(
                         "wdog_watchdogRefsContainer",
                         LE_WDOG_HASTABLE_WIDTH,
                         le_hashmap_HashUInt32,
                         le_hashmap_EqualsUInt32
                       );
    LE_ASSERT(WatchdogRefsContainer != NULL);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start up the watchdog server.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    InitializeTimerContainer();
    SystemProcessNotifySupervisor();
    wdog_ConnectService();

    le_msg_AddServiceCloseHandler (le_wdog_GetServiceRef(), CleanUpClosedClient, NULL);
    LE_INFO("The watchdog service is ready");
}
