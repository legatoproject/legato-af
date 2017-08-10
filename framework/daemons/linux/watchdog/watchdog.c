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
 *
 * The watchdog daemon can also be connected to an external watchdog daemon by registering
 * for the ExternalWatchdog event.  The registered handler will be called periodically if
 * all watchdogs are running.  If any watchdog is not running, the system will be rebooted, so
 * the external watchdog period should be set somewhat less than the hardware watchdog period
 * to allow time for the system to shutdown cleanly.
 */
//--------------------------------------------------------------------------------------------------
 /** @section wd_tedious_detail            More involved discussion follows
 *
 * The watchdog runs as a service which monitors critical processes on the system to check
 * if they are alive, and takes corrective action, such as restarting the process,  if not.
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
 * Normally a process can change its watchdog timeout by giving a timeout to le_wdog_Kick().  For
 * critical processes a maximum timeout should be given so the process cannot accidentally disable
 * the watchdog.  This can be given in the adef file in a maximumWatchdogTimeout section.
 *
 * @note If a maximumWatchdogTimeout is given the watchdog for the process will always be running,
 * even if the application is stopped.  Such applications should be started automatically, and
 * have a faultAction and watchdogAction which restarts the process.
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
 * @note Critical systems rely on the watchdog daemon to ensure system liveness, so all
 * unrecoverable errors in the watchdogDaemon are considered fatal to the system, and will
 * cause a system reboot by calling LE_FATAL or LE_ASSERT.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------
#include "legato.h"
#include "limit.h"
#include "interfaces.h"
#include "user.h"
#include "fileDescriptor.h"


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
 * The name of the node in the config tree that contains the list of processes for the application.
 *
 * If this entry in the config tree is missing or empty the application will not be launched.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_PROC_LIST                              "procs"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the maximum timeout for processes with
 * mandatory watchdogs.
 *
 * If this node is empty the process does not have a mandatory watchdog.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_MANDATORY_WDOG                         "maxWatchdogTimeout"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the default timeout for processes with
 * a watchdog
 *
 * If this node is empty the default watchdog timeout is used.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_WDOG_TIMEOUT                         "watchdogTimeout"


//--------------------------------------------------------------------------------------------------
/**
 * Size of the watchdog hash table.  Roughly equal to the expected number of watchdog users
 * (le_hashmap will take care of load factors).
 **/
//--------------------------------------------------------------------------------------------------
#define LE_WDOG_HASTABLE_WIDTH  31

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
 * Define a special PID to use for no such process.
 */
//--------------------------------------------------------------------------------------------------
#define NO_PROC      -1

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
    le_clk_Time_t maxKickTimeoutInterval; ///< Maximum timeout for this watchdog -- only used for
                                        ///< mandatory watchdogs but present everywhere so a
                                        ///< mandatory watchdog will not accidentally get set
                                        ///< beyond it's maximum period by being treated as a
                                        ///< non-mandatory watchdog.
    le_timer_Ref_t timer;               ///< The timer this watchdog uses
}
WatchdogObj_t;

//--------------------------------------------------------------------------------------------------
/**
 * Uniquely identifies a process in the system.
 *
 * Used as a key for the mandatory watchdog hash map.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uid_t appId;                                  ///< Application ID
    char procName[LIMIT_MAX_PROCESS_NAME_BYTES];  ///< Process Name
}
AppProcKey_t;

//--------------------------------------------------------------------------------------------------
/**
 * Mandatory watchdog definition
 *
 * Mandatory watchdogs are never completely deleted.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    WatchdogObj_t watchdog;               ///< The common watchdog definitions
    AppProcKey_t key;                     ///< The key in mandatory watchdog hash map
    char appName[LIMIT_MAX_APP_NAME_BYTES]; ///< Store the app name as the UID will no longer exist
                                            ///< when an app is uninstalled.
}
MandatoryWatchdogObj_t;

//--------------------------------------------------------------------------------------------------
/**
 * Define hashing functions for mandatory watchdog key
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_MAKE_HASH(AppProcKey_t)

//--------------------------------------------------------------------------------------------------
/**
 * External watchdog definitions
 *
 * Each external watchdog will be tickled periodically if all watchdogs are running.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{

    le_wdog_ExternalWatchdogHandlerFunc_t handler;
    void* contextPtr;
    le_timer_Ref_t timer;
}
ExternalWatchdogObj_t;

static le_mem_PoolRef_t WatchdogPool;           ///< The memory pool the watchdogs will come from
static le_hashmap_Ref_t WatchdogRefsContainer;  ///< The container we use to keep track of wdogs

static le_mem_PoolRef_t MandatoryWatchdogPool;  ///< The memory pool the mandatory watchdogs come
                                                ///< from
static le_hashmap_Ref_t MandatoryWatchdogRefs;  ///< The container used to track mandatory watchdogs

static le_mem_PoolRef_t ExternalWatchdogPool;   ///< The memory pool external for watchdog handlers

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
        // Give the watchdog one more kick if it hasn't had one, then release it.
        // This allows mandatory watchdogs (which still exist in the MandatoryWatchdogRefs
        // one more kick to restart before they're considered expired.
        if (deadDogPtr->procId >= 0)
        {
            deadDogPtr->procId = NO_PROC;
            le_timer_SetContextPtr(deadDogPtr->timer, (void*)((intptr_t)NO_PROC));
            le_timer_Start(deadDogPtr->timer);
        }
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


    if (procId == NO_PROC)
    {
        // Mandatory watchdog expired without the process restarting.  Restart Legato.
        LE_FATAL("A mandatory watchdog expired");
    }

    WatchdogObj_t* expiredDog = LookupClientWatchdogPtrById(procId);
    if (expiredDog != NULL)
    {
        uid_t appId = expiredDog->appId;


        if (LE_OK == le_appInfo_GetName(procId, appName, sizeof(appName) ))
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
 * Check a regular watchdog is running.
 */
//--------------------------------------------------------------------------------------------------
bool CheckWatchdog
(
    const void* keyPtr,
    const void* valuePtr,
    void* contextPtr
)
{
    bool* kickPtr = contextPtr;
    const WatchdogObj_t* dogPtr = valuePtr;

    if (   (!dogPtr->timer)
        || (!le_timer_IsRunning(dogPtr->timer)))
    {
        // Invalid state -- no process or timer is not running.
        *kickPtr = false;
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check a mandatory watchdog is running.
 */
//--------------------------------------------------------------------------------------------------
bool CheckMandatoryWatchdog
(
    const void* keyPtr,
    const void* valuePtr,
    void* contextPtr
)
{
    const MandatoryWatchdogObj_t* dogPtr = valuePtr;

    // Checking mandatory watchdogs is the same as a regular watchdog.  This is done in addition
    // to regular watchdog checking to ensure there are no stopped mandatory watchdogs which have
    // been removed from the regular watchdog list.
    return CheckWatchdog(&(dogPtr->watchdog.procId),
                         &(dogPtr->watchdog),
                         contextPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * The handler for external watchdog kicks.
 *
 * Check to ensure all timers are running, and if so kick the external watchdog(s).
 */
//--------------------------------------------------------------------------------------------------
static void ExternalWatchdogHandler
(
    le_timer_Ref_t timerRef
)
{
    bool kick = true;
    ExternalWatchdogObj_t* externalWdogPtr;

    externalWdogPtr = le_timer_GetContextPtr(timerRef);

    if ((!externalWdogPtr) ||
        (!externalWdogPtr->handler))
    {
        LE_FATAL("Invalid external watchdog.");
    }

    // Check both watchdogs and mandatory watchdogs -- this will double count most mandatory
    // watchdogs since all running mandatory are also in the WatchdogRefContainer, but we need
    // to check if any mandatory watchdogs have expired.
    if (le_hashmap_ForEach(WatchdogRefsContainer,
                           CheckWatchdog,
                           &kick) &&
        le_hashmap_ForEach(MandatoryWatchdogRefs,
                           CheckMandatoryWatchdog,
                           &kick) &&
        kick)
    {
        // Kick the external watchdog
        externalWdogPtr->handler(externalWdogPtr->contextPtr);
    }
    else
    {
        // Watchdog daemon or a mandatory watchdog is not functioning properly.  Exit
        // so we can cleanly restart the board before the hardware watchdog expires.
        LE_FATAL("One or more watchdogs have failed.");
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
    uint64_t milliseconds
)
{
    le_clk_Time_t interval;

    interval.sec = milliseconds / 1000;
    interval.usec = (milliseconds - (interval.sec * 1000)) * 1000;

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
    char pathStr[LIMIT_MAX_PATH_BYTES] = {0};
    char procPathStr[LIMIT_MAX_PATH_BYTES] = {0};

    if (name != NULL)
    {

        // on linux, /proc/[pid]/cmdline contains the command and arguments separated by '\0's
        int result = snprintf(pathStr, sizeof(pathStr), "/proc/%d/cmdline", pId);
        if (result < 0 || result >= LIMIT_MAX_PATH_BYTES)
        {
            return LE_NOT_FOUND;
        }

        int fd = -1;

        do
        {
            fd = open(pathStr, O_RDONLY);
        }
        while ((fd == -1) && (errno == EINTR));

        if (fd < 0)
        {
            LE_ERROR("Unable to open '%s': %m", pathStr);
            return LE_FAULT;
        }
        else
        {
            result = read (fd, procPathStr, LIMIT_MAX_PATH_BYTES);
            fd_Close(fd);

            if (result <= 0)
            {
                LE_ERROR("Unable to read '%s': %m", procPathStr);
                return LE_FAULT;
            }
            else if (strnlen(procPathStr, LIMIT_MAX_PATH_BYTES) ==  LIMIT_MAX_PATH_BYTES)
            {
                // We need the first parameter of the command line, which is path to a process.
                // This shouldn't be longer than LIMIT_MAX_PATH_BYTES.
                return LE_OVERFLOW;
            }

            // Set NULL at the end of the string
            if (LIMIT_MAX_PATH_BYTES == result)
            {
                procPathStr[result-1] = '\0';
            }
            else
            {
                procPathStr[result] = '\0';
            }

            // strip the path
            char* procNamePtr = le_path_GetBasenamePtr(procPathStr, "/");

            return le_utf8_Copy(name, procNamePtr, length, NULL);
        }

        fd_Close(fd);
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

    if (LE_OK == le_appInfo_GetName(procId, appName, sizeof(appName) ))
    {    // Check if there is a config for the process name first else check under the app name

        // It's a real app. Let's look up the config!
        LE_DEBUG("Getting configured watchdog timeout for app %s", appName);
        if (le_path_Concat("/", configPath, sizeof(configPath), CFG_NODE_APPS_LIST, appName,
                CFG_NODE_WDOG_TIMEOUT, NULL) == LE_OK)
        {
            app_milliseconds = le_cfg_QuickGetInt(configPath, CFG_TIMEOUT_USE_DEFAULT);
        }

        if (LE_OK == GetProcessNameFromPid( procId, procName, sizeof(procName)))
        {
            // get the config
            configPath[0]='\0';
            LE_DEBUG("Getting configured watchdog timeout for process %s", procName);

            if(le_path_Concat("/", configPath, sizeof(configPath),
                              CFG_NODE_APPS_LIST, appName,
                              CFG_NODE_PROC_LIST, procName,
                              CFG_NODE_WDOG_TIMEOUT, NULL) == LE_OK)
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
        LE_WARN("Unknown app with pid %d requested watchdog - using default timeout %d ms", procId,
          proc_milliseconds);
    }
    return MakeTimerInterval(proc_milliseconds);
}

//--------------------------------------------------------------------------------------------------
/**
 * Construct a new already allocated watchdog.
 */
//--------------------------------------------------------------------------------------------------
static void InitNewWatchdog
(
    WatchdogObj_t* newDogPtr,
    pid_t clientPid,
    uid_t appId,
    le_clk_Time_t kickTimeoutInterval,
    le_clk_Time_t maxKickTimeoutInterval
)
{
    char timerName[LIMIT_MAX_TIMER_NAME_BYTES];

    newDogPtr->procId = clientPid;
    newDogPtr->appId = appId;
    newDogPtr->kickTimeoutInterval = kickTimeoutInterval;
    newDogPtr->maxKickTimeoutInterval = maxKickTimeoutInterval;
    if (le_clk_GreaterThan(newDogPtr->kickTimeoutInterval, newDogPtr->maxKickTimeoutInterval))
    {
        newDogPtr->kickTimeoutInterval = newDogPtr->maxKickTimeoutInterval;
    }

    if (clientPid < 0)
    {
        // If no current client, just use address as identifier.  This is the case for
        // mandatory watchdogs as their process is not yet created.
        LE_ASSERT(0 <= snprintf(timerName, sizeof(timerName),
                                "wdog_p%d:%p", appId, newDogPtr));
    }
    else
    {
        LE_ASSERT(0 <= snprintf(timerName, sizeof(timerName), "wdog_u%d:p%d", clientPid, appId));
    }
    newDogPtr->timer = le_timer_Create(timerName);
    _Static_assert (sizeof(pid_t) <= sizeof(intptr_t), "pid_t is truncated by cast to void*");
    LE_ASSERT(LE_OK == le_timer_SetContextPtr(newDogPtr->timer, (void*)((intptr_t)clientPid)));
    LE_ASSERT(LE_OK == le_timer_SetHandler(newDogPtr->timer, WatchdogHandleExpiry));
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
    uid_t appId        ///< the user id of the client
)
{
    AppProcKey_t key;
    MandatoryWatchdogObj_t* mandatoryWdogPtr;
    WatchdogObj_t* newDogPtr = NULL;
    le_clk_Time_t maxKickTimeoutInterval;

    // First see if there's a mandatory watchdog
    memset(&key, 0, sizeof(key));
    key.appId = appId;
    LE_ASSERT(LE_OK == GetProcessNameFromPid( clientPid, key.procName, sizeof(key.procName)));
    mandatoryWdogPtr = le_hashmap_Get(MandatoryWatchdogRefs, &key);
    if (mandatoryWdogPtr)
    {
        // Use the mandatory watchdog
        LE_DEBUG("Attaching %d to mandatory watchdog", clientPid);
        newDogPtr = &(mandatoryWdogPtr->watchdog);
        le_mem_AddRef(mandatoryWdogPtr);
        // Stop the timer -- mandatory timers are always running, even if process
        // doesn't exist.
        le_timer_Stop(newDogPtr->timer);
        // Then update the proc ID to point to this new process.
        LE_ASSERT(LE_OK == le_timer_SetContextPtr(newDogPtr->timer,
                                                  (void*)((intptr_t)clientPid)));
        newDogPtr->procId = clientPid;
    }
    else
    {
        // Create a new watchdog
        LE_DEBUG("Making a new dog for %d", clientPid);
        newDogPtr = le_mem_ForceAlloc(WatchdogPool);
        maxKickTimeoutInterval = MakeTimerInterval(LE_WDOG_TIMEOUT_NEVER);
        InitNewWatchdog(newDogPtr, clientPid, appId,
                        GetConfigKickTimeoutInterval(clientPid, appId),
                        maxKickTimeoutInterval);
    }

    return newDogPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clean up an existing watchdog.
 */
//--------------------------------------------------------------------------------------------------
static void CleanupWdog
(
    void* objectPtr
)
{
    WatchdogObj_t* deadDogPtr = objectPtr;

    // If this watchdog has a timer, delete it.
    if (deadDogPtr->timer)
    {
        le_timer_Delete(deadDogPtr->timer);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Allocate a new manadatory watchdog object, construct it, and add it to the list of
 * mandatory watchdogs.
 */
//--------------------------------------------------------------------------------------------------
static void CreateMandatoryWatchdog
(
    const char* appNamePtr,
    const char* procNamePtr,
    uint64_t maxWatchdogTimeout
)
{
    MandatoryWatchdogObj_t* newDogPtr = le_mem_ForceAlloc(MandatoryWatchdogPool);
    uid_t appId;
    le_clk_Time_t maxWatchdogTime = MakeTimerInterval(maxWatchdogTimeout);

    memset(newDogPtr, 0, sizeof(MandatoryWatchdogObj_t));
    LE_ASSERT(LE_OK == user_GetAppUid(appNamePtr, &appId));
    newDogPtr->key.appId = appId;
    strncpy(newDogPtr->key.procName, procNamePtr, sizeof(newDogPtr->key.procName));

    // Create watchdog setting initial timeout to max timeout.  This allows the maximum
    // time for the application to start.
    InitNewWatchdog(&(newDogPtr->watchdog), NO_PROC, appId,
                    maxWatchdogTime, maxWatchdogTime);

    LE_INFO("Creating new mandatory watchdog for %d[%s]",
             newDogPtr->key.appId, newDogPtr->key.procName);
    LE_ASSERT(NULL == le_hashmap_Put(MandatoryWatchdogRefs, &(newDogPtr->key), newDogPtr));

    // Immediately start this watchdog.
    LE_ASSERT(LE_OK == le_timer_SetInterval(newDogPtr->watchdog.timer,
                                            newDogPtr->watchdog.kickTimeoutInterval));
    le_timer_Start(newDogPtr->watchdog.timer);
}

//--------------------------------------------------------------------------------------------------
/**
 * Do not allow mandatory watchdogs to be deleted unless the app has been uninstalled.
 * That's what makes them mandatory.
 */
//--------------------------------------------------------------------------------------------------
static void CleanupMandatoryWdog
(
    void* objPtr
)
{
    MandatoryWatchdogObj_t* newDogPtr = objPtr;
    uid_t appUid;
    le_result_t result;

    result = user_GetAppUid(newDogPtr->appName, &appUid);
    if (result == LE_NOT_FOUND)
    {
        LE_INFO("Removing mandatory watchdog for %s[%s]",
                newDogPtr->appName, newDogPtr->key.procName);
    }
    else
    {
        LE_FATAL("Cannot destroy mandatory watchdog for %s[%s]",
                 newDogPtr->appName, newDogPtr->key.procName);
    }
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
            if (le_clk_GreaterThan(timeoutValue, watchDogPtr->maxKickTimeoutInterval))
            {
                LE_WARN("Capping watchdog timeout for process [%d] app (%d) to maximum of %lu.%lds"
                        " (was %lu.%lds).",
                        watchDogPtr->procId, watchDogPtr->appId,
                        watchDogPtr->maxKickTimeoutInterval.sec,
                        watchDogPtr->maxKickTimeoutInterval.usec,
                        timeoutValue.sec,
                        timeoutValue.usec);

                timeoutValue = watchDogPtr->maxKickTimeoutInterval;
            }
        }

        if (!le_clk_Equal(timeoutValue, MakeTimerInterval(LE_WDOG_TIMEOUT_NEVER)))
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
 * Register a function to be called to kick an external watchdog.
 */
//--------------------------------------------------------------------------------------------------
le_wdog_ExternalWatchdogHandlerRef_t le_wdog_AddExternalWatchdogHandler
(
    int32_t milliseconds,
    le_wdog_ExternalWatchdogHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    ExternalWatchdogObj_t* externalWdogPtr = le_mem_ForceAlloc(ExternalWatchdogPool);

    externalWdogPtr->handler = handlerPtr;
    externalWdogPtr->contextPtr = contextPtr;
    externalWdogPtr->timer = le_timer_Create("ExternalWdog");
    if (!externalWdogPtr->timer)
    {
        LE_ERROR("Failed to create external watchdog timer");
        le_mem_Release(externalWdogPtr);
        return NULL;
    }
    le_timer_SetHandler(externalWdogPtr->timer, ExternalWatchdogHandler);
    le_timer_SetContextPtr(externalWdogPtr->timer, externalWdogPtr);
    le_timer_SetRepeat(externalWdogPtr->timer, 0); // repeat indefinitely
    le_timer_SetMsInterval(externalWdogPtr->timer, milliseconds);

    if (LE_OK != le_timer_Start(externalWdogPtr->timer))
    {
        LE_ERROR("Failed to start external watchdog timer");
        le_timer_Delete(externalWdogPtr->timer);
        le_mem_Release(externalWdogPtr);
        return NULL;
    }

    return (le_wdog_ExternalWatchdogHandlerRef_t)externalWdogPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_wdog_ExternalWatchdog'
 */
//--------------------------------------------------------------------------------------------------
void le_wdog_RemoveExternalWatchdogHandler
(
    le_wdog_ExternalWatchdogHandlerRef_t handlerRef
        ///< [IN]
)
{
    ExternalWatchdogObj_t* externalWdogPtr = (ExternalWatchdogObj_t*)handlerRef;

    le_timer_Delete(externalWdogPtr->timer);
    le_mem_Release(externalWdogPtr);
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
    le_mem_SetDestructor(WatchdogPool, CleanupWdog);
    WatchdogRefsContainer = le_hashmap_Create(
                         "wdog_watchdogRefsContainer",
                         LE_WDOG_HASTABLE_WIDTH,
                         le_hashmap_HashUInt32,
                         le_hashmap_EqualsUInt32
                       );
    LE_ASSERT(WatchdogRefsContainer != NULL);

    MandatoryWatchdogPool = le_mem_CreatePool("MandatoryWdogPool", sizeof(MandatoryWatchdogObj_t));
    le_mem_SetDestructor(MandatoryWatchdogPool, CleanupMandatoryWdog);
    MandatoryWatchdogRefs = le_hashmap_Create(
        "wdog_mandatoryWatchdogRefs",
        LE_WDOG_HASTABLE_WIDTH,
        HashAppProcKey_t,
        EqualsAppProcKey_t);
    LE_ASSERT(NULL != MandatoryWatchdogRefs);
    le_hashmap_MakeTraceable(MandatoryWatchdogRefs);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize all processes in an app with a mandatory watchdog kick
 */
//--------------------------------------------------------------------------------------------------
static void InitMandatoryWdogForApp
(
    le_cfg_IteratorRef_t appCfg
)
{
    // Get the app name.
    char appName[LIMIT_MAX_APP_NAME_BYTES];
    uint32_t appWatchdogTimeout = 0;

    if (le_cfg_GetNodeName(appCfg, "", appName, sizeof(appName)) == LE_OVERFLOW)
    {
        LE_WARN("AppName buffer was too small, name truncated to '%s'.  "
                "Max app name in bytes, %d.",
                appName, LIMIT_MAX_APP_NAME_BYTES);
        LE_INFO("If this app has a mandatory watchdog, the system will fail.");
    }

    appWatchdogTimeout = le_cfg_GetInt(appCfg, CFG_NODE_MANDATORY_WDOG, 0);

    le_cfg_GoToNode(appCfg, CFG_NODE_PROC_LIST);

    if (LE_OK != le_cfg_GoToFirstChild(appCfg))
    {
        LE_WARN("No processes in app");
        le_cfg_GoToParent(appCfg);
        return;
    }

    do
    {
        // Get the process name.
        char procName[LIMIT_MAX_PROCESS_NAME_BYTES];

        if (le_cfg_GetNodeName(appCfg, "", procName, sizeof(appName)) == LE_OVERFLOW)
        {
            // Failing to create a mandatory watchdog is fatal.
            LE_FATAL("ProcName buffer was too small, name truncated to '%s'.  "
                     "Max process name in bytes, %d.",
                     procName, LIMIT_MAX_PROCESS_NAME_BYTES);
        }

        // Get the watchdog timeout for this process.  Use the app's timeout if there's no
        // process-specific one.
        uint32_t wdogTimeout = le_cfg_GetInt(appCfg, CFG_NODE_MANDATORY_WDOG, appWatchdogTimeout);
        if (wdogTimeout)
        {
            // Get the process name.
            char procName[LIMIT_MAX_PROCESS_NAME_BYTES];

            if (le_cfg_GetNodeName(appCfg, "", procName, sizeof(appName)) == LE_OVERFLOW)
            {
                // Failing to create a mandatory watchdog is fatal.
                LE_FATAL("ProcName buffer was too small, name truncated to '%s'.  "
                         "Max process name in bytes, %d.",
                         procName, LIMIT_MAX_PROCESS_NAME_BYTES);
            }

            CreateMandatoryWatchdog(appName, procName, wdogTimeout);
        }
    }
    while (LE_OK == le_cfg_GoToNextSibling(appCfg));

    // Get back up to app level.
    le_cfg_GoToParent(appCfg);
    le_cfg_GoToParent(appCfg);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle app install.  Create all mandatory watchdogs for this app.
 */
//--------------------------------------------------------------------------------------------------
void HandleAppInstall
(
    const char* appName,
    void* contextPtr
)
{
    char appCfgPath[LIMIT_MAX_PATH_BYTES];
    le_cfg_IteratorRef_t appCfg;

    snprintf(appCfgPath, sizeof(appCfgPath),
             "/" CFG_NODE_APPS_LIST "/%s", appName);

    appCfg = le_cfg_CreateReadTxn(appCfgPath);

    InitMandatoryWdogForApp(appCfg);

    le_cfg_CancelTxn(appCfg);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle app uninstall.  Remove all mandatory watchdogs from this app.
 */
//--------------------------------------------------------------------------------------------------
void HandleAppUninstall
(
    const char* appName,
    void* contextPtr
)
{
    le_hashmap_It_Ref_t mandatoryWdogIterator;
    le_result_t result;

    mandatoryWdogIterator = le_hashmap_GetIterator(MandatoryWatchdogRefs);
    result = le_hashmap_NextNode(mandatoryWdogIterator);

    while (LE_OK == result)
    {
        // Get the watchdog to be examined, and immediately move to the next entry in case
        // this one is deleted
        MandatoryWatchdogObj_t* mandatoryWdogPtr = le_hashmap_GetValue(mandatoryWdogIterator);
        result = le_hashmap_NextNode(mandatoryWdogIterator);

        if (0 == strcmp(mandatoryWdogPtr->appName, appName))
        {
            // This watchdog belongs to the app which has just been uninstalled; remove it.
            LE_ASSERT(LE_OK == le_hashmap_Remove(MandatoryWatchdogRefs, &(mandatoryWdogPtr->key)));
            le_mem_Release(mandatoryWdogPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize apps/processes with mandatory watchdog kicks.
 */
//--------------------------------------------------------------------------------------------------
static void InitMandatoryWdog
(
    void
)
{
    // Read the list of applications from the config tree.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(CFG_NODE_APPS_LIST);


    // Register for app install/uninstall so the mandatory watchdog tables can be kept up to
    // date.
    LE_ASSERT(le_instStat_AddAppInstallEventHandler(HandleAppInstall, NULL));
    LE_ASSERT(le_instStat_AddAppUninstallEventHandler(HandleAppUninstall, NULL));

    if (le_cfg_GoToFirstChild(appCfg) != LE_OK)
    {
        LE_WARN("No applications installed.");

        le_cfg_CancelTxn(appCfg);

        return;
    }

    // Go through each application and initialize any processes which have mandatory watchdogs.
    do
    {
        InitMandatoryWdogForApp(appCfg);
    }
    while (LE_OK == le_cfg_GoToNextSibling(appCfg));

    le_cfg_CancelTxn(appCfg);
}


//--------------------------------------------------------------------------------------------------
/**
 * Start up the watchdog server.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    InitializeTimerContainer();
    ExternalWatchdogPool = le_mem_CreatePool("ExternalWatchdogPool", sizeof(ExternalWatchdogObj_t));
    LE_ASSERT(NULL != ExternalWatchdogPool);

    SystemProcessNotifySupervisor();
    wdog_ConnectService();
    le_appInfo_ConnectService();

    InitMandatoryWdog();

    le_msg_AddServiceCloseHandler (le_wdog_GetServiceRef(), CleanUpClosedClient, NULL);
    LE_INFO("The watchdog service is ready");
}
