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

#define DEBUG

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
 *    "<legato-prefix>_<tag>_<client-pid>"
 *
 * Maximum name length is therefore:
 *    - Length of prefix, plus
 *    - Maximum tag length, plus
 *    - Number of decimal digits in MAX_INT (10), plus
 *    - Two characters for underscore ("_") and one for string termination
 */
//--------------------------------------------------------------------------------------------------
#define LEGATO_WS_NAME_FORMAT LEGATO_TAG_PREFIX"_%s_%d"
#define LEGATO_WS_NAME_LEN (sizeof(LEGATO_TAG_PREFIX) + LE_PM_TAG_LEN + 10 + 3)
///@}

///@{
//--------------------------------------------------------------------------------------------------
/**
 * Memory pool sizes
 */
//--------------------------------------------------------------------------------------------------
#define CLIENT_DEFAULT_POOL_SIZE 8
#define WAKEUP_SOURCE_DEFAULT_POOL_SIZE 64
///@}

//--------------------------------------------------------------------------------------------------
/**
 * Wakeup source record definition
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
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
typedef struct {
    uint32_t cookie;              // used to validate pointer to Client_t
    pid_t pid;                    // client pid
    le_msg_SessionRef_t session;  // back-reference to client connect session
} Client_t;
#define PM_CLIENT_COOKIE 0x7732c691

//--------------------------------------------------------------------------------------------------
/**
 * Global power manager record
 */
//--------------------------------------------------------------------------------------------------
static struct {
    int                 wl;      // file descriptor of /sys/power/wake_lock
    int                 wu;      // file descriptor of /sys/power/wake_unlock
    le_ref_MapRef_t     refs;    // safe references to wakeup source objects
    le_mem_PoolRef_t    lpool;   // memory pool for wakeup source records
    le_hashmap_Ref_t    locks;   // table of wakeup source records
    le_mem_PoolRef_t    cpool;   // memory pool for client records
    le_hashmap_Ref_t    clients; // table of client records
}
PowerManager = {-1, -1, NULL, NULL, NULL, NULL, NULL};

//--------------------------------------------------------------------------------------------------
/**
 * Type-cast from le_pm_WakeupSourceRef_t to WakeupSource_t
 *
 */
//--------------------------------------------------------------------------------------------------
static inline WakeupSource_t *ToWakeupSource(le_pm_WakeupSourceRef_t w)
{
    WakeupSource_t *ws = (WakeupSource_t *)le_ref_Lookup(PowerManager.refs, w);
    if (!ws)
        LE_KILL_CLIENT("Error: bad wakeup source reference %p.", w);

#ifdef DEBUG
    if (PM_WAKEUP_SOURCE_COOKIE != ws->cookie || ws->wsref != w)
        LE_FATAL("Error: invalid wakeup source %p.", w);
#endif

    return ws;
}

//--------------------------------------------------------------------------------------------------
/**
 * Type-cast from void * (client table record pointer) to Client_t
 *
 */
//--------------------------------------------------------------------------------------------------
#ifdef DEBUG
static inline Client_t *to_Client_t(void *c)
{
    Client_t *cl = (Client_t *)c;
    if (!cl || PM_CLIENT_COOKIE != cl->cookie)
        LE_FATAL("Error: bad client %p.", c);

    return cl;
}
#else
#define to_Client_t(c) ((Client_t*)c)
#endif

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

    // Allocate and populate client record (exits on error)
    c = (Client_t*)le_mem_ForceAlloc(PowerManager.cpool);
    c->cookie = PM_CLIENT_COOKIE;
    c->session = sessionRef;
    if (LE_OK != le_msg_GetClientProcessId(sessionRef, &c->pid))
        LE_FATAL("Error getting client pid.");

    // Store client record in table
    if (le_hashmap_Put(PowerManager.clients, sessionRef, c))
        LE_FATAL("Error adding client record for pid %d.", c->pid);

    LE_INFO("Connection from client pid = %d.", c->pid);
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
    LE_INFO("Client pid %d disconnected.", c->pid);

    // Find and remove all wakeup sources held for this client
    iter = le_hashmap_GetIterator(PowerManager.locks);
    while (LE_OK == le_hashmap_NextNode(iter)) {
        ws = (WakeupSource_t*)le_hashmap_GetValue(iter);
        if (ws->pid != c->pid)
            // Does not belong to this client, skip
            continue;

        // Release wakeup source if taken
        if (ws->taken) {
            LE_WARN("Releasing wakeup source '%s' on behalf of pid %d.",
                    ws->name, ws->pid);
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
        LE_FATAL("Failed to open %s, errno = %d.", WAKE_LOCK_FILE, errno);

    // Open wake unlock file
    PowerManager.wu = open(WAKE_UNLOCK_FILE, O_RDWR);
    if (-1 == PowerManager.wu)
        LE_FATAL("Failed to open %s, errno = %d.", WAKE_UNLOCK_FILE, errno);

    // Create table of safe references
    PowerManager.refs = le_ref_CreateMap("PM References", 31);
    if (NULL == PowerManager.refs)
        LE_FATAL("Failed to create safe reference table");

    // Create memory pool for wakeup source records - exits on error
    PowerManager.lpool = le_mem_CreatePool("PM Wakeup Source Mem Pool",
                                           sizeof(WakeupSource_t));
    le_mem_ExpandPool(PowerManager.lpool, WAKEUP_SOURCE_DEFAULT_POOL_SIZE);

    // Create table of wakeup sources
    PowerManager.locks = le_hashmap_Create("PM Wakeup Sources",
                                           31,
                                           le_hashmap_HashString,
                                           le_hashmap_EqualsString);
    if (NULL == PowerManager.locks)
        LE_FATAL("Failed to create wakeup source hashmap");

    // Create memory pool for client records - exits on error
    PowerManager.cpool = le_mem_CreatePool("PM Client Mem Pool",
                                           sizeof(Client_t));
    le_mem_ExpandPool(PowerManager.cpool, CLIENT_DEFAULT_POOL_SIZE);

    // Create table of clients
    PowerManager.clients = le_hashmap_Create("PM Clients",
                                             31,
                                             le_hashmap_HashVoidPointer,
                                             le_hashmap_EqualsVoidPointer);
    if (NULL == PowerManager.clients)
        LE_FATAL("Failed to create client hashmap");

    // Register client connect/disconnect handlers
    le_msg_AddServiceOpenHandler(le_pm_GetServiceRef(), OnClientConnect, NULL);
    le_msg_AddServiceCloseHandler(le_pm_GetServiceRef(), OnClientDisconnect, NULL);

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
le_pm_WakeupSourceRef_t le_pm_NewWakeupSource(uint32_t opts, const char *tag)
{
    WakeupSource_t *ws;
    Client_t *cl;
    char name[LEGATO_WS_NAME_LEN];

    if (!tag || *tag == '\0' || strlen(tag) > LE_PM_TAG_LEN) {
        LE_KILL_CLIENT("Error: Tag value is invalid or NULL.");
        return NULL;
    }

    // Find and validate client record
    cl = to_Client_t(le_hashmap_Get(PowerManager.clients,
                                    le_pm_GetClientSessionRef()));

    // Check if identical wakeup source already exists for this client
    sprintf(name, LEGATO_WS_NAME_FORMAT, tag, cl->pid);

    // Lookup wakeup source by name
    ws = (WakeupSource_t*)le_hashmap_Get(PowerManager.locks, name);
    if (ws) {
        LE_KILL_CLIENT("Error: Tag '%s' already exists.", tag);
        return NULL;
    }

    // Allocate and populate wakeup source record (exits on error)
    ws = (WakeupSource_t*)le_mem_ForceAlloc(PowerManager.lpool);
    ws->cookie = PM_WAKEUP_SOURCE_COOKIE;
    strcpy(ws->name, name);
    ws->taken = 0;
    ws->pid = cl->pid;
    ws->isRef = (opts & LE_PM_REF_COUNT ? true : false);

    ws->wsref = le_ref_CreateRef(PowerManager.refs, ws);

    // Store record in table of wakeup sources
    if (le_hashmap_Put(PowerManager.locks, ws->name, ws))
        LE_FATAL("Error adding wakeup source '%s'.", ws->name);

    LE_INFO("Created new wakeup source '%s' for pid %d.", ws->name, ws->pid);

    return (le_pm_WakeupSourceRef_t)ws->wsref;
}

//--------------------------------------------------------------------------------------------------
/**
 * Acquire a wakeup source
 *
 * @note The process exits on failures
 */
//--------------------------------------------------------------------------------------------------
void le_pm_StayAwake(le_pm_WakeupSourceRef_t w)
{
    WakeupSource_t *ws, *entry;

    // Validate the reference, check if it exists
    ws = ToWakeupSource(w);
    // If the wakeup source is NULL then the client will have
    // been killed and we can just return
    if (NULL == ws)
        return;

    entry = (WakeupSource_t*)le_hashmap_Get(PowerManager.locks, ws->name);
    if (!entry)
        LE_FATAL("Wakeup source '%s' not created.\n", ws->name);

    if (entry->taken++) {
        if (!entry->isRef) {
            LE_WARN("Wakeup source '%s' already acquired.", entry->name);
        }
        if (0 == entry->taken) {
            LE_KILL_CLIENT("Wakeup source '%s' reference counter overlaps.", entry->name);
        }
        return;
    }

    // Write to /sys/power/wake_lock
    if (0 > write(PowerManager.wl, entry->name, strlen(entry->name)))
        LE_FATAL("Error acquiring wakeup soruce '%s', errno = %d.",
            entry->name, errno);

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release a wakeup source
 *
 * @note The process exits on failure
 */
//--------------------------------------------------------------------------------------------------
void le_pm_Relax(le_pm_WakeupSourceRef_t w)
{
    WakeupSource_t *ws, *entry;

    // Validate the reference, check if it exists
    ws = ToWakeupSource(w);
    // If the wakeup source is NULL then the client will have
    // been killed and we can just return
    if (NULL == ws)
        return;

    entry = (WakeupSource_t*)le_hashmap_Get(PowerManager.locks, ws->name);
    if (!entry)
        LE_FATAL("Wakeup source '%s' not created.\n", ws->name);

    if (!entry->taken) {
        LE_ERROR("Wakeup source '%s' already released.", entry->name);
        return;
    }

    entry->taken--;
    if (entry->isRef) {
        if (UINT_MAX == entry->taken) {
            LE_KILL_CLIENT("Wakeup source '%s' reference counter overlaps.", entry->name);
        }
        if (entry->taken > 0) {
           return;
        }
    }
    else {
        entry->taken = 0;
    }

    // write to /sys/power/wake_unlock
    if (0 > write(PowerManager.wu, entry->name, strlen(entry->name)))
        LE_FATAL("Error releasing wakeup soruce '%s', errno = %d.",
            entry->name, errno);

    return;
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
    while (le_hashmap_NextNode(iter) == LE_OK) {
        wakeSrc = (WakeupSource_t*)le_hashmap_GetValue(iter);

        if (wakeSrc->taken) {
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
