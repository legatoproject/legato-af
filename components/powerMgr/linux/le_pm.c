//--------------------------------------------------------------------------------------------------
/**
 * @file le_pm.c
 *
 * This file contains the source code of the top level Power Management API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pm.h"
#include "watchdogChain.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

///@{
//--------------------------------------------------------------------------------------------------
/**
 * Power Management sysfs interface files
 */
//--------------------------------------------------------------------------------------------------
#define WAKE_LOCK_FILE      "/sys/power/wake_lock"
#define WAKE_UNLOCK_FILE    "/sys/power/wake_unlock"
///@}

//--------------------------------------------------------------------------------------------------
/**
 * Legato's prefix for wakeup source names
 */
//--------------------------------------------------------------------------------------------------
#define LEGATO_TAG_PREFIX   "legato"

///@{
//--------------------------------------------------------------------------------------------------
/**
 * Format of Legato wakeup source names:
 *    "<legato-prefix>_<tag>_<client-process-name>"
 *
 * Maximum name length is therefore:
 *    - Length of prefix, plus
 *    - Maximum tag length, plus
 *    - Maximum client process name, plus
 *    - Two characters for underscore ("_") and one for string termination
 */
//--------------------------------------------------------------------------------------------------
#define LEGATO_WS_NAME_FORMAT LEGATO_TAG_PREFIX"_%s_%s"
#define LEGATO_WS_PROCNAME_LEN 30
#define LEGATO_WS_NAME_LEN (sizeof(LEGATO_TAG_PREFIX) + LE_PM_TAG_LEN + LEGATO_WS_PROCNAME_LEN + 3)
///@}

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 8

///@{
//--------------------------------------------------------------------------------------------------
/**
 * Memory pool sizes
 */
//--------------------------------------------------------------------------------------------------
#define CLIENT_DEFAULT_POOL_SIZE 8
#define CLIENT_DEFAULT_HASH_SIZE 31
#define WAKEUP_SOURCE_DEFAULT_POOL_SIZE 64
#define PM_REFERENCE_DEFAULT_POOL_SIZE   31
///@}

//--------------------------------------------------------------------------------------------------
/**
 * Wakeup source record definition
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t      cookie;   // used to validate pointer to WakeupSource_t
    char          name[LEGATO_WS_NAME_LEN];    // full wakeup source name
    uint32_t      taken;    // > 0 locked, 0 = unlocked
    pid_t         pid;      // client pid of wakeup source owner
    void          *wsref;   // back-pointer to safe reference
    bool          isRef;     // true if reference counted, false if not
}
WakeupSource_t;
#define PM_WAKEUP_SOURCE_COOKIE 0xa1f6337b

//--------------------------------------------------------------------------------------------------
/**
 * Client record definition
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t cookie;                         // used to validate pointer to Client_t
    pid_t pid;                               // client pid
    le_msg_SessionRef_t session;             // back-reference to client connect session
    char name[LEGATO_WS_PROCNAME_LEN + 1];   // client process name
}
Client_t;
#define PM_CLIENT_COOKIE 0x7732c691

//--------------------------------------------------------------------------------------------------
/**
 * Global power manager record
 */
//--------------------------------------------------------------------------------------------------
static struct
{
    int                 wl;      // file descriptor of /sys/power/wake_lock
    int                 wu;      // file descriptor of /sys/power/wake_unlock
    le_ref_MapRef_t     refs;    // safe references to wakeup source objects
    le_mem_PoolRef_t    lpool;   // memory pool for wakeup source records
    le_hashmap_Ref_t    locks;   // table of wakeup source records
    le_mem_PoolRef_t    cpool;   // memory pool for client records
    le_hashmap_Ref_t    clients; // table of client records
    bool                isFull;  // le_pm_StayAwke() fails with LE_NO_MEMORY
}
PowerManager = {-1, -1, NULL, NULL, NULL, NULL, NULL, false};

//--------------------------------------------------------------------------------------------------
/**
 * Define static reference map
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(PMReferences, PM_REFERENCE_DEFAULT_POOL_SIZE);

//--------------------------------------------------------------------------------------------------
/**
 * Define static pool for wakeup sources.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(PMSource,
                          WAKEUP_SOURCE_DEFAULT_POOL_SIZE,
                          sizeof(WakeupSource_t));

//--------------------------------------------------------------------------------------------------
/**
 * Define static hashmap for wakeup sources
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(PMWakeupSources,
                         WAKEUP_SOURCE_DEFAULT_POOL_SIZE);

//--------------------------------------------------------------------------------------------------
/**
 * Define static pool for client records.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(PMClient,
                          CLIENT_DEFAULT_POOL_SIZE,
                          sizeof(Client_t));

//--------------------------------------------------------------------------------------------------
/**
 * Define static hash map for client records.
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(PMClient,
                         CLIENT_DEFAULT_HASH_SIZE);


//--------------------------------------------------------------------------------------------------
/**
 * Type-cast from le_pm_WakeupSourceRef_t to WakeupSource_t
 *
 */
//--------------------------------------------------------------------------------------------------
static inline WakeupSource_t *ToWakeupSource
(
    le_pm_WakeupSourceRef_t w
)
{
    WakeupSource_t *ws = (WakeupSource_t *)le_ref_Lookup(PowerManager.refs, w);
    if (NULL == ws)
    {
        LE_KILL_CLIENT("Error: bad wakeup source reference %p.", w);
        return NULL;
    }

#ifdef DEBUG
    if (PM_WAKEUP_SOURCE_COOKIE != ws->cookie || ws->wsref != w)
    {
        LE_FATAL("Error: invalid wakeup source %p.", w);
    }
#endif

    return ws;
}

//--------------------------------------------------------------------------------------------------
/**
 * Type-cast from void * (client table record pointer) to Client_t
 *
 */
//--------------------------------------------------------------------------------------------------
#define DEBUG
#ifdef DEBUG
static inline Client_t *to_Client_t
(
    void *c
)
{
    Client_t *cl = (Client_t *)c;
    if (!cl || PM_CLIENT_COOKIE != cl->cookie)
    {
        LE_FATAL("Error: bad client %p.", c);
    }

    return cl;
}
#else
#define to_Client_t(c) ((Client_t*)c)
#endif
#undef DEBUG

//--------------------------------------------------------------------------------------------------
/**
 * Client connect callback
 *
 */
//--------------------------------------------------------------------------------------------------
static void OnClientConnect
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    Client_t *c;
    FILE *procFd;
    char procStr[PATH_MAX];
    size_t procLen;

    // Allocate and populate client record (exits on error)
    c = (Client_t*)le_mem_ForceAlloc(PowerManager.cpool);
    c->cookie = PM_CLIENT_COOKIE;
    c->session = sessionRef;
    if (LE_OK != le_msg_GetClientProcessId(sessionRef, &c->pid))
    {
        LE_FATAL("Error getting client pid.");
    }

    snprintf(procStr, sizeof(procStr), "/proc/%d/comm", c->pid);
    procFd = fopen(procStr, "r");
    if (NULL == procFd)
    {
        LE_FATAL("Error when opening process %d command line: %m", c->pid);
    }

    // Get the process name
    if (NULL == fgets(procStr, sizeof(procStr), procFd))
    {
        LE_FATAL("Error when scanning process %d command line", c->pid);
    }
    fclose(procFd);
    procLen = strlen(procStr);
    procStr[procLen - 1] = '\0';
    memset(c->name, 0, sizeof(c->name));
    le_utf8_Copy(c->name, procStr, sizeof(c->name), NULL);

    // Store client record in table
    if (le_hashmap_Put(PowerManager.clients, sessionRef, c))
    {
        LE_FATAL("Error adding client record for pid %d.", c->pid);
    }

    LE_INFO("Connection from client %s/%d", c->name, c->pid);
}

//--------------------------------------------------------------------------------------------------
/**
 * Client disconnect callback
 *
 */
//--------------------------------------------------------------------------------------------------
static void OnClientDisconnect
(
    le_msg_SessionRef_t sessionRef,
    void *contextPtr
)
{
    Client_t *c;
    WakeupSource_t *ws;
    le_hashmap_It_Ref_t iter;

    // Find and remove client record from table
    c = to_Client_t(le_hashmap_Remove(PowerManager.clients, sessionRef));

    if (c == NULL)
    {
        LE_ERROR("Cannot remove sessionRef %p from table.", sessionRef);
        return;
    }

    LE_INFO("Client pid %d disconnected.", c->pid);

    // Find and remove all wakeup sources held for this client
    iter = le_hashmap_GetIterator(PowerManager.locks);
    while (LE_OK == le_hashmap_NextNode(iter))
    {
        ws = (WakeupSource_t*)le_hashmap_GetValue(iter);
        if (ws->pid != c->pid)
        {
            // Does not belong to this client, skip
            continue;
        }

        // Release wakeup source if taken
        if (ws->taken)
        {
            LE_WARN("Releasing wakeup source '%s' on behalf of %s/%d.",
                    ws->name, c->name, ws->pid);
            // Force the wakeup source to be released discarding the reference count
            ws->isRef = false;
            le_pm_Relax((le_pm_WakeupSourceRef_t)ws->wsref);
        }

        // Delete wakeup source record, free memory
        LE_INFO("Deleting wakeup source '%s' on behalf of pid %d.",
                ws->name, ws->pid);
        le_hashmap_Remove(PowerManager.locks, ws->name);
        le_ref_DeleteRef(PowerManager.refs, ws->wsref);
        le_mem_Release(ws);
    }

    // Free client record
    le_mem_Release(c);

    return;
}


//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Power Manager component
 *
 * @note The process exits on failure
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Atomic initialization: initialize all items or fail
    // Open wake lock file
    PowerManager.wl = open(WAKE_LOCK_FILE, O_RDWR);
    if (-1 == PowerManager.wl)
    {
        LE_FATAL("Failed to open %s: %m.", WAKE_LOCK_FILE);
    }

    // Open wake unlock file
    PowerManager.wu = open(WAKE_UNLOCK_FILE, O_RDWR);
    if (-1 == PowerManager.wu)
    {
        LE_FATAL("Failed to open %s: %m.", WAKE_UNLOCK_FILE);
    }

    // Create table of safe references
    PowerManager.refs = le_ref_InitStaticMap(PMReferences, PM_REFERENCE_DEFAULT_POOL_SIZE);
    if (NULL == PowerManager.refs)
    {
        LE_FATAL("Failed to create safe reference table");
    }

    // Create memory pool for wakeup source records - exits on error
    PowerManager.lpool = le_mem_InitStaticPool(PMSource,
                                               WAKEUP_SOURCE_DEFAULT_POOL_SIZE,
                                           sizeof(WakeupSource_t));

    // Create table of wakeup sources
    PowerManager.locks = le_hashmap_InitStatic(PMWakeupSources,
                                               WAKEUP_SOURCE_DEFAULT_POOL_SIZE,
                                           le_hashmap_HashString,
                                           le_hashmap_EqualsString);
    if (NULL == PowerManager.locks)
    {
        LE_FATAL("Failed to create wakeup source hashmap");
    }

    // Create memory pool for client records - exits on error
    PowerManager.cpool = le_mem_InitStaticPool(PMClient,
                                               CLIENT_DEFAULT_POOL_SIZE,
                                           sizeof(Client_t));

    // Create table of clients
    PowerManager.clients = le_hashmap_InitStatic(PMClient,
                                                 CLIENT_DEFAULT_HASH_SIZE,
                                             le_hashmap_HashVoidPointer,
                                             le_hashmap_EqualsVoidPointer);
    if (NULL == PowerManager.clients)
    {
        LE_FATAL("Failed to create client hashmap");
    }

    // Register client connect/disconnect handlers
    le_msg_AddServiceOpenHandler(le_pm_GetServiceRef(), OnClientConnect, NULL);
    le_msg_AddServiceCloseHandler(le_pm_GetServiceRef(), OnClientDisconnect, NULL);

    // Releasing all "Legato" wakeup sources remaining from a previous powerMgr daemon
    FILE *wlFd;

    wlFd = fopen(WAKE_LOCK_FILE, "r");
    if (wlFd)
    {
        char wsStr[LEGATO_WS_NAME_LEN * 2];
        char wsScanFormatStr[10]; // Dynamically build the format string for fscanf
        int rc;

        snprintf(wsScanFormatStr, sizeof(wsScanFormatStr), "%%%zus ", sizeof(wsStr));
        while (1 == fscanf(wlFd, wsScanFormatStr, wsStr))
        {
            if (0 == strncmp(wsStr, LEGATO_TAG_PREFIX "_", sizeof(LEGATO_TAG_PREFIX)))
            {
                LE_INFO("Releasing wakeup source '%s'", wsStr);
                rc = write(PowerManager.wu, wsStr, strlen(wsStr));
                (void)rc;
            }
        }
        snprintf(wsScanFormatStr, sizeof(wsScanFormatStr), "%%%zus", sizeof(wsStr));
        if ((1 == fscanf(wlFd, wsScanFormatStr, wsStr)) &&
            (0 == strncmp(wsStr, LEGATO_TAG_PREFIX "_", sizeof(LEGATO_TAG_PREFIX))))
        {
            LE_INFO("Releasing wakeup source '%s'", wsStr);
            rc = write(PowerManager.wu, wsStr, strlen(wsStr));
            (void)rc;
        }
        fclose(wlFd);
    }

    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);

    // We're up and running
    LE_INFO("Power Manager service is running.");

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a new wakeup source
 *
 * @return Reference to wakeup source, NULL on failure
 *
 * @note The process exits on syscall failures
 */
//--------------------------------------------------------------------------------------------------
le_pm_WakeupSourceRef_t le_pm_NewWakeupSource
(
    uint32_t opts,
    const char *tag
)
{
    WakeupSource_t *ws;
    Client_t *cl;
    char name[LEGATO_WS_NAME_LEN];

    if (('\0' == *tag) || (strlen(tag) > LE_PM_TAG_LEN))
    {
        LE_KILL_CLIENT("Error: Tag value is invalid or NULL.");
        return NULL;
    }

    // Find and validate client record
    cl = to_Client_t(le_hashmap_Get(PowerManager.clients,
                                    le_pm_GetClientSessionRef()));

    if (cl == NULL)
    {
        LE_ERROR("Cannot find client record.");
        return NULL;
    }

    // Check if identical wakeup source already exists for this client
    snprintf(name, sizeof(name), LEGATO_WS_NAME_FORMAT, tag, cl->name);

    // Lookup wakeup source by name
    ws = (WakeupSource_t*)le_hashmap_Get(PowerManager.locks, name);
    if (ws)
    {
        LE_KILL_CLIENT("Error: Tag '%s' already exists.", tag);
        return NULL;
    }

    // Allocate and populate wakeup source record (exits on error)
    ws = (WakeupSource_t*)le_mem_ForceAlloc(PowerManager.lpool);
    ws->cookie = PM_WAKEUP_SOURCE_COOKIE;
    le_utf8_Copy(ws->name, name, sizeof(ws->name), NULL);
    ws->taken = 0;
    ws->pid = cl->pid;
    ws->isRef = (opts & LE_PM_REF_COUNT ? true : false);

    ws->wsref = le_ref_CreateRef(PowerManager.refs, ws);

    // Store record in table of wakeup sources
    if (le_hashmap_Put(PowerManager.locks, ws->name, ws))
    {
        LE_FATAL("Error adding wakeup source '%s'.", ws->name);
    }

    LE_INFO("Created new wakeup source '%s' for pid %d.", ws->name, ws->pid);

    return (le_pm_WakeupSourceRef_t)ws->wsref;
}

//--------------------------------------------------------------------------------------------------
/**
 * Acquire a wakeup source
 *
 * @return
 *     - LE_OK          if the wakeup source is acquired
 *     - LE_NO_MEMORY   if the wakeup sources limit is reached
 *     - LE_FAULT       for other errors
 *
 * @note The process exits if an invalid reference is passed
 * @note The wakeup sources limit is fixed by the kernel CONFIG_PM_WAKELOCKS_LIMIT configuration
 *       variable
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pm_StayAwake
(
    le_pm_WakeupSourceRef_t w
)
{
    WakeupSource_t *ws, *entry;

    // Validate the reference, check if it exists
    ws = ToWakeupSource(w);
    // If the wakeup source is NULL then the client will have
    // been killed and we can just return
    if (NULL == ws)
    {
        return LE_OK;
    }

    entry = (WakeupSource_t*)le_hashmap_Get(PowerManager.locks, ws->name);
    if (!entry)
    {
        LE_KILL_CLIENT("Wakeup source '%s' not created.\n", ws->name);
        return LE_OK;
    }

    if (entry->taken++)
    {
        if (!entry->isRef)
        {
            LE_WARN("Wakeup source '%s' already acquired.", entry->name);
        }
        if (0 == entry->taken)
        {
            LE_KILL_CLIENT("Wakeup source '%s' reference counter overlaps.", entry->name);
        }
        return LE_OK;
    }

    // Write to /sys/power/wake_lock
    if (0 > write(PowerManager.wl, entry->name, strlen(entry->name)))
    {
        if (ENOSPC == errno)
        {
            LE_ERROR("Too many wakeup source: Cannot acquire '%s'.", entry->name);
            PowerManager.isFull = true;
            return LE_NO_MEMORY;
        }
        else if (EBADF == errno)
        {
            LE_FATAL("Error acquiring wakeup source '%s'. Invalid file descriptor %d.",
                     entry->name, PowerManager.wl);
        }
        else
        {
            LE_CRIT("Error acquiring wakeup source '%s': %m", entry->name);
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release a previously acquired wakeup source
 *
 * @return
 *     - LE_OK          if the wakeup source is acquired
 *     - LE_NOT_FOUND   if the wakeup source was not currently acquired
 *     - LE_FAULT       for other errors
 *
 * @note The process exits if an invalid reference is passed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pm_Relax
(
    le_pm_WakeupSourceRef_t w
)
{
    WakeupSource_t *ws, *entry;

    // Validate the reference, check if it exists
    ws = ToWakeupSource(w);
    // If the wakeup source is NULL then the client will have
    // been killed and we can just return
    if (NULL == ws)
    {
        return LE_OK;
    }

    entry = (WakeupSource_t*)le_hashmap_Get(PowerManager.locks, ws->name);
    if (!entry)
    {
        LE_KILL_CLIENT("Wakeup source '%s' not created.\n", ws->name);
        return LE_OK;
    }

    if (!entry->taken)
    {
        LE_ERROR("Wakeup source '%s' already released.", entry->name);
        return LE_OK;
    }

    entry->taken--;
    if (entry->isRef)
    {
        if (UINT_MAX == entry->taken)
        {
            LE_KILL_CLIENT("Wakeup source '%s' reference counter overlaps.", entry->name);
        }
        if (entry->taken > 0)
        {
           return LE_OK;
        }
    }
    else
    {
        entry->taken = 0;
    }

    // write to /sys/power/wake_unlock
    if (0 > write(PowerManager.wu, entry->name, strlen(entry->name)))
    {
        if (EINVAL == errno)
        {
            LE_ERROR("Wakeup source '%s' is not locked.", entry->name);
            return LE_NOT_FOUND;
        }
        else if (EBADF == errno)
        {
            LE_FATAL("Error releasing wakeup source '%s'. Invalid file descriptor %d.",
                     entry->name, PowerManager.wu);
        }
        else
        {
            LE_CRIT("Error releasing wakeup source '%s': %m", entry->name);
            return LE_FAULT;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether any process is holding a wakelock.
 *
 * @return true if any process is holding a wakelock, false otherwise.
 *
 */
//--------------------------------------------------------------------------------------------------
bool pm_CheckWakeLock
(
    void
)
{
    WakeupSource_t *wakeSrc;
    le_hashmap_It_Ref_t iter;
    bool wakelockHeld = false;

    //Traverse all wakeup sources and check for wakelocks.
    iter = le_hashmap_GetIterator(PowerManager.locks);
    while (le_hashmap_NextNode(iter) == LE_OK)
    {
        wakeSrc = (WakeupSource_t*)le_hashmap_GetValue(iter);

        if (wakeSrc->taken)
        {
            // Wakelock held
            LE_DEBUG("Wakelock held(Pid: %d, Wake Source Name: %s)",
                     wakeSrc->pid,
                     wakeSrc->name);
            wakelockHeld = true;
            break;
        }
    }

    return wakelockHeld;
}


//--------------------------------------------------------------------------------------------------
/**
 * Release and destroy all acquired wakeup source, kill all clients
 *
 * @return
 *     - LE_OK              if the wakeup source is acquired
 *     - LE_NOT_PERMITTED   if the le_pm_StayAwake() has not failed with LE_NO_MEMORY
 *     - LE_FAULT           for other errors
 *
 * @note The service is available only if le_pm_StayAwake() has returned LE_NO_MEMORY. It should be
 *       used in the way to release and destroy all wakeup sources.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pm_ForceRelaxAndDestroyAllWakeupSource
(
    void
)
{
    Client_t *c;
    le_hashmap_It_Ref_t iter;

    if (!PowerManager.isFull)
    {
        LE_ERROR("Service is not permitted at this time.");
        return LE_NOT_PERMITTED;
    }

    // Find and remove client record from table
    iter = le_hashmap_GetIterator(PowerManager.clients);
    while (LE_OK == le_hashmap_NextNode(iter))
    {
        c = to_Client_t(le_hashmap_GetValue(iter));
        LE_INFO("Client %s/%d killed.", c->name, c->pid);
        le_msg_CloseSession(c->session);
    }

    PowerManager.isFull = false;

    return LE_OK;
}
