/** @file updateCtrl.c
 *
 * For useage and information on updateCtrl see @le_updateCtrl.api
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "limit.h"
#include "interfaces.h"
#include "sysStatus.h"
#include "system.h"
#include "updateCtrl.h"

// Save the lock/defer states so they don't have to be recomputed on every query.
static int AggregateProbationLockCount = 0;     ///< current count of locks held by all clients
static int AggregateDeferCount = 0;             ///< count of all client update defers

//--------------------------------------------------------------------------------------------------
/**
 * Update Dameon will set this callback if it tries to expire probation while it is locked.
 * We callback if/when all prbation locks are revoked.
 */
//--------------------------------------------------------------------------------------------------
static void (*ProbationExpiryCallback)(void) = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * If the updateDaemon tries to fail/rollback the system while a defer is in effect, it will set
 * this callback. When the defer is lifted we will call back to let the fail proceed.
 */
//--------------------------------------------------------------------------------------------------
static void (*DeferredFailCallback)(void) = NULL;

#define UPDATECTRL_HASTABLE_WIDTH  31   ///< Width of the hash table (s/b prime)

typedef struct
{
    pid_t ClientPID;                    ///< Hash key (so we don't need to allocate it separately)
    int LockProbationCount;             ///< Number of Probation Locks this client holds.
    int DeferCount;                     ///< Number of Defer Locks this client holds.
}
ClientLockCountObj_t;

//--------------------------------------------------------------------------------------------------
/*
 * Set up storage for keeping track of client lock counts.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t UpdateCtrlPool;             ///< Mem pool to alloc lock count objects from.
static le_hashmap_Ref_t ClientLockCountsContainer;  ///< Container used to store lock count objects.

//--------------------------------------------------------------------------------------------------
/**
 * Set a callback to call for when all probation locks are removed.
 */
//--------------------------------------------------------------------------------------------------
void updateCtrl_SetProbationExpiryCallBack
(
    void (*probationExpiryCallback)(void)
)
{
    ProbationExpiryCallback = probationExpiryCallback;
}

//--------------------------------------------------------------------------------------------------
/**
 * When a client dies deallocate any storage we allocated to store its counts.
 *
 * @note
 * If a client dies while holding probation-locks, it is better to let the framework reboot (to
 * avoid marking system good despite of failure) at its earliest convenience. However, if a client
 * dies while holding defer-locks, it is probably OK to release the defer-locks (held by dead client)
 * without reboot.
 */
//--------------------------------------------------------------------------------------------------
static void FreeDeadClientLockObjs
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    pid_t clientPid;

    if (LE_OK == le_msg_GetClientUserCreds(sessionRef, NULL, &clientPid))
    {
        ClientLockCountObj_t* obj = le_hashmap_Remove(ClientLockCountsContainer, &clientPid);
        {
            if (obj)
            {
                if (obj->LockProbationCount != 0)
                {
                    LE_FATAL("Process %d died while holding %d probation locks",
                              clientPid,
                              obj->LockProbationCount);
                }
                else
                {
                    if (obj->DeferCount != 0)
                    {
                        LE_EMERG("Process %d died while holding %d defer locks",
                                  clientPid,
                                  obj->DeferCount);

                        AggregateDeferCount -= obj->DeferCount;
                    }

                    le_mem_Release(obj);
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the mem pool so we can allocate our counter objects and set up the hash we will
 * store them in. This will be done in a lazy way. We will only do the work of initializing if
 * someone actually want to use this.
 */
//--------------------------------------------------------------------------------------------------
void updateCtrl_Initialize
(
    void
)
{
    UpdateCtrlPool = le_mem_CreatePool("UpdateCtrlPool", sizeof(ClientLockCountObj_t));
    ClientLockCountsContainer = le_hashmap_Create(
                         "UpdateCtrl_ClientLockCountsContainer",
                         UPDATECTRL_HASTABLE_WIDTH,
                         le_hashmap_HashUInt32,
                         le_hashmap_EqualsUInt32
                       );
    LE_ASSERT(ClientLockCountsContainer != NULL);
    le_msg_AddServiceCloseHandler (le_updateCtrl_GetServiceRef(), FreeDeadClientLockObjs, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Construct a clean client lock count object
 */
//--------------------------------------------------------------------------------------------------
static ClientLockCountObj_t* CreateClientLockCountObj
(
    void
)
{
    ClientLockCountObj_t* obj;
    obj = le_mem_ForceAlloc(UpdateCtrlPool);
    obj->ClientPID = 0;
    obj->LockProbationCount = 0;
    obj->DeferCount = 0;
    return obj;
}


//--------------------------------------------------------------------------------------------------
/**
 * If the pid has a lock object, increment the probation locks, else create a lock object for the
 * process and set the probation lock to 1.
 */
//--------------------------------------------------------------------------------------------------
static void IncrementProbationLocks
(
    pid_t clientPid
)
{
    ClientLockCountObj_t* obj = le_hashmap_Get(ClientLockCountsContainer, &clientPid);
    if (obj == NULL)
    {
        obj = CreateClientLockCountObj();
        obj->ClientPID = clientPid;
        LE_ASSERT(NULL == le_hashmap_Put(ClientLockCountsContainer, &(obj->ClientPID), obj));
    }
    obj->LockProbationCount ++;
    AggregateProbationLockCount++;
}

//--------------------------------------------------------------------------------------------------
/**
 * If the pid has a probation lock count, decrement it. If the lock object for the process now
 * has no counts for either probation or defers, delete it and free its storage.
 */
//--------------------------------------------------------------------------------------------------
static void DecrementProbationLocks
(
    pid_t clientPid
)
{
    ClientLockCountObj_t* obj = le_hashmap_Get(ClientLockCountsContainer, &clientPid);

    if (obj == NULL)
    {
        LE_KILL_CLIENT("Client PID %d is trying to unset a Probation Lock but never set one.", clientPid);
    }
    else
    {
        if (obj->LockProbationCount <= 0)
        {
            LE_KILL_CLIENT("Client PID %d has unset more Probation Locks than it set", clientPid);
        }
        else
        {
            obj->LockProbationCount --;
            AggregateProbationLockCount--;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Does the client hold a probation lock?
 *
 * @return true if the client holds a probation lock
 */
//--------------------------------------------------------------------------------------------------
static bool IsClientProbationLocked
(
    pid_t clientPid
)
{
    ClientLockCountObj_t* obj = le_hashmap_Get(ClientLockCountsContainer, &clientPid);
    return (obj != NULL) && (obj->LockProbationCount > 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * If the pid has a lock object, increment the defer count, else create a lock object for the
 * process and set the defer coutn to 1.
 */
//--------------------------------------------------------------------------------------------------
static void IncrementDefers
(
    pid_t clientPid
)
{
    ClientLockCountObj_t* obj = le_hashmap_Get(ClientLockCountsContainer, &clientPid);
    if (obj == NULL)
    {
        obj = CreateClientLockCountObj();
        obj->ClientPID = clientPid;
        LE_ASSERT(NULL == le_hashmap_Put(ClientLockCountsContainer, &(obj->ClientPID), obj));
    }
    obj->DeferCount ++;
    AggregateDeferCount++;
}

//--------------------------------------------------------------------------------------------------
/**
 * If the pid has a probation defer count, decrement it. If the lock object for the process now
 * has no counts for either probation or defers, delete it and free its storage.
 */
//--------------------------------------------------------------------------------------------------
static void DecrementDefers
(
    pid_t clientPid
)
{
    ClientLockCountObj_t* obj = le_hashmap_Get(ClientLockCountsContainer, &clientPid);
    if (obj == NULL)
    {
        LE_KILL_CLIENT("Client PID %d is trying to unset a Defer but never set one.", clientPid);
    }
    else
    {
        if (obj->DeferCount <= 0)
        {
            LE_KILL_CLIENT("Client PID %d has unset more Defers than it set", clientPid);
        }
        else
        {
            obj->DeferCount --;
            AggregateDeferCount--;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Does the client have a defer count?
 *
 * @return true if the client is deferring updates
 */
//--------------------------------------------------------------------------------------------------
static bool IsClientDeferring
(
    pid_t clientPid
)
{
    ClientLockCountObj_t* obj = le_hashmap_Get(ClientLockCountsContainer, &clientPid);
    return (obj != NULL) && (obj->DeferCount > 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Check for defer counts for any processes that we have records for.
 * @return
 *      - true    At least one process holds at least one defer.
 *      - false   No defers are found.
 */
//--------------------------------------------------------------------------------------------------
bool updateCtrl_HasDefers
(
    void
)
{
    return (AggregateDeferCount > 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the PID of the calling client
 */
//--------------------------------------------------------------------------------------------------
static pid_t GetClientPid
(
    void
)
{
    uid_t clientUserId = 0;
    pid_t clientProcId = 0;
    le_msg_SessionRef_t sessionRef = le_updateCtrl_GetClientSessionRef();

    if (LE_OK == le_msg_GetClientUserCreds(sessionRef, &clientUserId, &clientProcId))
    {
        LE_INFO("the pid is %d", clientProcId);
        // and add the client
    }
    else
    {
        LE_WARN("Can't find client Id. The client may have closed the session.");
    }
    return clientProcId;
}

//--------------------------------------------------------------------------------------------------
/**
 * Called from the probation timout handler to determine whether it is OK to mark the system "good".
 *
 * @return
 *      - true    If probation is locked
 *      - false   Probation is not locked
 */
//--------------------------------------------------------------------------------------------------
bool updateCtrl_IsProbationLocked
(
    void
)
{
    return AggregateProbationLockCount > 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Prevent all updates (and roll-backs) until further notice.
 */
//--------------------------------------------------------------------------------------------------
    // Do we need to remember if anyone tries an update so it can pend?
void le_updateCtrl_Defer
(
    void
)
{
    pid_t clientPid = GetClientPid();
    // 0 is not a real pid and if the pid isn't real, don't do anything!
    if ( clientPid != 0)
    {
        IncrementDefers(clientPid);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Allow updates to go ahead.
 */
//--------------------------------------------------------------------------------------------------
void le_updateCtrl_Allow
(
    void
)
{
    pid_t clientPid = GetClientPid();
    // 0 is not a real pid and if the pid isn't real, don't do anything!
    if ( clientPid != 0)
    {
        DecrementDefers(clientPid);
        if (!updateCtrl_HasDefers())
        {
            if (DeferredFailCallback)
            {
                DeferredFailCallback();
                // In the current implementation execution won't reach here - but we clear this
                // callback anyway just in case this changes in the future.
                DeferredFailCallback = NULL;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Prevent the probation period from ending.
 *
 * @note Ignored if the probation period has already ended.
 */
//--------------------------------------------------------------------------------------------------
bool le_updateCtrl_LockProbation
(
    void
)
{
    pid_t clientPid = GetClientPid();
    // 0 is not a real pid and if the pid isn't real, don't do anything!
    if ( clientPid != 0)
    {
        if (sysStatus_Status() == SYS_PROBATION)
        {
            IncrementProbationLocks(clientPid);
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Cancels a call to LockProbation(), allow the probation period to end.
 *
 * @note Ignored if the probation period has already ended.
 */
//--------------------------------------------------------------------------------------------------
void le_updateCtrl_UnlockProbation
(
    void
)
{
    pid_t clientPid = GetClientPid();
    // 0 is not a real pid and if the pid isn't real, don't do anything!
    if ( clientPid != 0)
    {
        DecrementProbationLocks(clientPid);
        if ( !updateCtrl_IsProbationLocked())
        {
            if (ProbationExpiryCallback)
            {
                ProbationExpiryCallback();
                ProbationExpiryCallback = NULL;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Log any processes holding probation locks
 */
//--------------------------------------------------------------------------------------------------
static bool LogLock
(
    const void* keyPtr,
    const void* valuePtr,
    void* contextPtr
)
{
    char pathStr[LIMIT_MAX_PATH_BYTES] = "";
    char procNameBuffer[LIMIT_MAX_PROCESS_NAME_BYTES] = "";
    char* procName = "unknown";
    ClientLockCountObj_t*  obj = (ClientLockCountObj_t*) valuePtr;

    int result = snprintf(pathStr, sizeof(pathStr), "/proc/%d/comm", obj->ClientPID);
    if (result <=  LIMIT_MAX_PATH_BYTES)
    {
        int fd = open(pathStr, O_RDONLY);
        if (fd >= 0)
        {
            result = read (fd, procNameBuffer, sizeof(procNameBuffer) - 1);
            close(fd);
            if (result > 0)
            {
                procNameBuffer[result] = '\0';
                procName = procNameBuffer;
            }
        }

    }

    LE_WARN(" - %s[%d] has %d probation locks.", procName, obj->ClientPID, obj->LockProbationCount);
    LE_WARN(" - %s[%d] has %d defers.", procName, obj->ClientPID, obj->DeferCount);
    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Marks the system "good", ending the probation period.
 *
 * @return
 *      - LE_OK         The system was marked Good
 *      - LE_BUSY       Someone holds a probation lock
 *      - LE_DUPLICATE  Probation has expired - the system has already been marked
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_updateCtrl_MarkGood
(
    bool force
)
{
    le_result_t result = LE_OK;

    if (sysStatus_Status() == SYS_PROBATION)
    {
        if ( !updateCtrl_IsProbationLocked() || force)
        {
            updateDaemon_MarkGood();
        }
        else
        {
            LE_WARN("Cannot mark Good. The following hold probation locks.");
            le_hashmap_ForEach(ClientLockCountsContainer, LogLock, NULL);
            result = LE_BUSY;
        }
    }
    else
    {
        result = LE_DUPLICATE;
    }
    return  result;
}

//--------------------------------------------------------------------------------------------------
/**
 * We have marked the current system bad. To begin the roll back all we do is restart the framework.
 */
//--------------------------------------------------------------------------------------------------
static void BeginRollBack
(
    void
)
{
    LE_FATAL("Last update defer revoked on a failed system. Initiating roll back.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Marks the system "bad" and triggers a roll-back to a "good" system.
 *
 * @note Ignored if the probation period has already ended.  Also, the roll-back may be delayed if
 * someone is deferring updates using le_updateCtrl_Defer().
 */
//--------------------------------------------------------------------------------------------------
void le_updateCtrl_FailProbation
(
    void
)
{
    if (sysStatus_Status() == SYS_PROBATION)
    {
        sysStatus_MarkBad();
        if (updateCtrl_HasDefers())
        {
            // we can't start the rollback just yet. Set a callback to be called when the last defer
            // is cancelled.
            DeferredFailCallback = BeginRollBack;
            LE_INFO("There is currently one or more defers in effect. " \
                        "Rollback will be called when defers are lifted");
        }
        else
        {
            LE_FATAL("System has been marked Bad. Rolling back.");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the current system state.
 *
 * @note Can only be called if updates have been deferred or if a probation lock is held.
 * Otherwise the system state could change between the time this function is called and when
 * the return value is checked.
 */
//--------------------------------------------------------------------------------------------------
le_updateCtrl_SystemState_t le_updateCtrl_GetSystemState
(
    void
)
{
    pid_t clientPid = GetClientPid();

    if ((!IsClientDeferring(clientPid)) &&
        (!IsClientProbationLocked(clientPid)))
    {
        LE_KILL_CLIENT("Client trying to get system state without lock.");
    }

    switch (sysStatus_Status())
    {
        case SYS_GOOD:
            return LE_UPDATECTRL_SYSTEMSTATE_GOOD;
        case SYS_BAD:
            return LE_UPDATECTRL_SYSTEMSTATE_BAD;
        case SYS_PROBATION:
            return LE_UPDATECTRL_SYSTEMSTATE_PROBATION;
        default:
            LE_FATAL("Unknown system status");
    }

    return LE_UPDATECTRL_SYSTEMSTATE_BAD;
}
