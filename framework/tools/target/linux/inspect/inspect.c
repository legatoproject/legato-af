//--------------------------------------------------------------------------------------------------
/** @file inspect.c
 *
 * Legato inspection tool used to inspect Legato structures such as memory pools, timers, threads,
 * mutexes, etc. in running processes.
 *
 * Must be run as root.
 *
 * @todo Add inspect by process name.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "mem.h"
#include "thread.h"
#include "messagingInterface.h"
#include "messagingProtocol.h"
#include "messagingSession.h"
#include "limit.h"
#include "addr.h"
#include "fileDescriptor.h"
#include "timer.h"

#include <sys/ptrace.h>

//--------------------------------------------------------------------------------------------------
/**
 * Insert a string name variable if configured or a placeholder string if not.
 *
 *  @param  nameVar Name variable to insert.
 *
 *  @return Name variable or a placeholder string depending on configuration.
 **/
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_THREAD_NAMES_ENABLED
#   define  THREAD_NAME(var)    (var)
#else
#   define  THREAD_NAME(var)    "<omitted>"
#endif
#if LE_CONFIG_TIMER_NAMES_ENABLED
#   define  TIMER_NAME(var) (var)
#else
#   define  TIMER_NAME(var) "<omitted>"
#endif
#if LE_CONFIG_MUTEX_NAMES_ENABLED
#   define  MUTEX_NAME(var) (var)
#else
#   define  MUTEX_NAME(var) "<omitted>"
#endif
#if LE_CONFIG_SEM_NAMES_ENABLED
#   define  SEM_NAME(var)   (var)
#else
#   define  SEM_NAME(var)   "<omitted>"
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Objects of these types are used to refer to lists of memory pools, thread objects, timers,
 * mutexes, semaphores, and service objects. They can be used to iterate over those lists in a
 * remote process.
 */
//--------------------------------------------------------------------------------------------------
typedef struct MemPoolIter*         MemPoolIter_Ref_t;
typedef struct ThreadObjIter*       ThreadObjIter_Ref_t;
typedef struct TimerIter*           TimerIter_Ref_t;
typedef struct MutexIter*           MutexIter_Ref_t;
typedef struct SemaphoreIter*       SemaphoreIter_Ref_t;
typedef struct ThreadMemberObjIter* ThreadMemberObjIter_Ref_t;
typedef struct ServiceObjIter*      ServiceObjIter_Ref_t;
typedef struct ClientObjIter*       ClientObjIter_Ref_t;
typedef struct SessionObjIter*      SessionObjIter_Ref_t;
typedef struct InterfaceObjIter*    InterfaceObjIter_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Inspection types - what's being inspected for the remote process.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    INSPECT_INSP_TYPE_MEM_POOL,
    INSPECT_INSP_TYPE_THREAD_OBJ,
    INSPECT_INSP_TYPE_TIMER,
    INSPECT_INSP_TYPE_MUTEX,
    INSPECT_INSP_TYPE_SEMAPHORE,
    INSPECT_INSP_TYPE_IPC_SERVERS,
    INSPECT_INSP_TYPE_IPC_CLIENTS,
    INSPECT_INSP_TYPE_IPC_SERVERS_SESSIONS,
    INSPECT_INSP_TYPE_IPC_CLIENTS_SESSIONS
}
InspType_t;


//--------------------------------------------------------------------------------------------------
/**
 * Object containing items necessary for accessing a doubly-linked list in the remote process.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_List_t List;         ///< The list in the remote process.
    size_t* ListChgCntRef;      ///< Change counter for the remote list.
    le_dls_Link_t* headLinkPtr; ///< Pointer to the first link.
}
RemoteDlsListAccess_t;


//--------------------------------------------------------------------------------------------------
/**
 * Object containing items necessary for accessing a singly-linked list in the remote process.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sls_List_t List;         ///< The list in the remote process.
    size_t* ListChgCntRef;      ///< Change counter for the remote list.
    le_sls_Link_t* headLinkPtr; ///< Pointer to the first link.
}
RemoteSlsListAccess_t;


//--------------------------------------------------------------------------------------------------
/**
 * Type of remote list access to use for hashmap lists
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_REDUCE_FOOTPRINT
typedef RemoteSlsListAccess_t RemoteHashmapListAccess_t;
#else
typedef RemoteDlsListAccess_t RemoteHashmapListAccess_t;
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Object containing items necessary for walking a hashmap in the remote process.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_hashmap_Bucket_t* bucketsPtr;  ///< Array of buckets in the hashmap in the remote process.
    size_t bucketCount;         ///< Size of the array of buckets.
    size_t* mapChgCntRef;       ///< Change counter for the remote map.
}
RemoteHashmapAccess_t;


//--------------------------------------------------------------------------------------------------
/**
 * Iterator objects for stepping through the list of memory pools, thread objects, timers, mutexes,
 * and semaphores in a remote process.
 */
//--------------------------------------------------------------------------------------------------
typedef struct MemPoolIter
{
    RemoteDlsListAccess_t memPoolList; ///< Memory pool list in the remote process.
    le_mem_Pool_t currMemPool;          ///< Current memory pool from the list.
}
MemPoolIter_t;

typedef struct ThreadObjIter
{
    RemoteDlsListAccess_t threadObjList; ///< Thread object list in the remote process.
    thread_Obj_t currThreadObj;        ///< Current thread object from the list.
}
ThreadObjIter_t;

typedef struct TimerIter
{
    RemoteDlsListAccess_t threadObjList;
    RemoteDlsListAccess_t timerList;     ///< Timer list for the current thread in the remote process.
    thread_Obj_t currThreadObj;
    Timer_t currTimer;                ///< Current timer from the list.
}
TimerIter_t;

typedef struct MutexIter
{
    RemoteDlsListAccess_t threadObjList;
    RemoteDlsListAccess_t mutexList;     ///< Mutexe list for the current thread in the remote process.
    thread_Obj_t currThreadObj;
    Mutex_t currMutex;                ///< Current mutex from the list.
}
MutexIter_t;

typedef struct SemaphoreIter
{
    RemoteDlsListAccess_t threadObjList;
    RemoteDlsListAccess_t semaphoreList; ///< This is a dummy, since there's no semaphore list.
    thread_Obj_t currThreadObj;
    Semaphore_t currSemaphore;        ///< Current semaphore from the list.
}
SemaphoreIter_t;

// Type describing the commonalities of the thread memeber objects - namely timer, mutex, and
// semaphore.
typedef struct ThreadMemberObjIter
{
    RemoteDlsListAccess_t threadObjList;
    RemoteDlsListAccess_t threadMemberObjList;
    thread_Obj_t currThreadObj;
}
ThreadMemberObjIter_t;

typedef struct ServiceObjIter
{
    RemoteHashmapAccess_t serviceObjMap; ///< Service object map in the remote process.
    size_t currIndex;                    ///< Current index in the bucket array.
    RemoteHashmapListAccess_t serviceObjList;
                                         ///< Service object list (technically a list of hashmap
                                         ///< entries containing pointers to service objects) of the
                                         ///< current bucket of the service object map in the remote
                                         ///< process.
    le_hashmap_Entry_t currEntry;                   ///< Current entry containing the service obj.
    msgInterface_UnixService_t currServiceObj; ///< Current service object from the list.
}
ServiceObjIter_t;

typedef struct ClientObjIter
{
    RemoteHashmapAccess_t clientObjMap;  ///< Client object map in the remote process.
    size_t currIndex;
    RemoteHashmapListAccess_t clientObjList;
                                         ///< Client object list (technically a list of hashmap
                                         ///< entries containing pointers to client objects) of the
                                         ///< current bucket of the client object map in the remote
                                         ///< process.
    le_hashmap_Entry_t currEntry;                   ///< Current entry containing the client obj.
    msgInterface_ClientInterface_t currClientObj; ///< Current client object from the list.
}
ClientObjIter_t;

typedef struct SessionObjIter
{
    RemoteHashmapAccess_t interfaceObjMap; ///< Interface object map in the remote process.
    size_t currIndex;
    RemoteHashmapListAccess_t interfaceObjList;
                                         ///< Interface object list (technically a list of hashmap
                                         ///< entries containing pointers to interface objects)
                                         ///< of the current bucket of the interface object map
                                         ///< in the remote process.
    le_hashmap_Entry_t currEntry;                   ///< Current entry containing the interface obj.
    RemoteDlsListAccess_t sessionList;      ///< Session list of the current interface obj.
    msgSession_UnixSession_t currSessionObj; ///< Current session object.
}
SessionObjIter_t;

// Type describing the commonalities of the interface objects - namely service, client, and session
// objects
typedef struct InterfaceObjIter
{
    RemoteHashmapAccess_t interfaceObjMap;
    size_t currIndex;
    RemoteHashmapListAccess_t interfaceObjList;
    le_hashmap_Entry_t currEntry;
}
InterfaceObjIter_t;


//--------------------------------------------------------------------------------------------------
/**
 * Local memory pool that is used for allocating an inspection object iterator.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t IteratorPool;


//--------------------------------------------------------------------------------------------------
/**
 * ASCII code for the escape character.
 */
//--------------------------------------------------------------------------------------------------
#define ESCAPE_CHAR         27


//--------------------------------------------------------------------------------------------------
/**
 * Default refresh interval in seconds.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_REFRESH_INTERVAL            3


//--------------------------------------------------------------------------------------------------
/**
 * Default retry interval in microseconds.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_RETRY_INTERVAL              500000


//--------------------------------------------------------------------------------------------------
/**
 * Variable storing the configurable refresh interval in seconds.
 */
//--------------------------------------------------------------------------------------------------
static time_t RefreshInterval = DEFAULT_REFRESH_INTERVAL;


//--------------------------------------------------------------------------------------------------
/**
 * Refresh timer for the interval and follow options
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t refreshTimer = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * PID of the process to inspect.
 */
//--------------------------------------------------------------------------------------------------
static pid_t PidToInspect = -1;


//--------------------------------------------------------------------------------------------------
/**
 * Indicating if the Inspect results are output as the JSON format or not. Currently false implies
 * a human-readable format.
 */
//--------------------------------------------------------------------------------------------------
static bool IsOutputJson = false;


//--------------------------------------------------------------------------------------------------
/**
 * Inspection type.
 **/
//--------------------------------------------------------------------------------------------------
InspType_t InspectType;


//--------------------------------------------------------------------------------------------------
/**
 * true = follow (periodically update the output until the program is killed with SIGINT or
 *        something).
 **/
//--------------------------------------------------------------------------------------------------
static bool IsFollowing = false;


//--------------------------------------------------------------------------------------------------
/**
 * true = verbose mode (everything is printed).
 **/
//--------------------------------------------------------------------------------------------------
static bool IsVerbose = false;


//--------------------------------------------------------------------------------------------------
/**
 * true = child process stopped
 */
//--------------------------------------------------------------------------------------------------
static bool IsChildStopped = false;


//--------------------------------------------------------------------------------------------------
/**
 * Local mapped address of liblegato.so
 */
//--------------------------------------------------------------------------------------------------
uintptr_t LocalLibLegatoBaseAddr = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Child mapped address of liblegato.so
 */
//--------------------------------------------------------------------------------------------------
uintptr_t ChildLibLegatoBaseAddr = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Signal to deliver when process is restarted
 */
//--------------------------------------------------------------------------------------------------
int PendingChildSignal = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Flags indicating how an inspection ended.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    INSPECT_SUCCESS,    ///< inspection completed without interruption or error.
    INSPECT_INTERRUPTED ///< inspection was interrupted due to list changes.
}
InspectEndStatus_t;


//--------------------------------------------------------------------------------------------------
/**
 * Prints a generic message on stderr so that the user is aware there is a problem, logs the
 * internal error message and exit.
 */
//--------------------------------------------------------------------------------------------------
#define INTERNAL_ERR(formatString, ...)                                                 \
            { fprintf(stderr, "Internal error check logs for details.\n");              \
              LE_FATAL(formatString, ##__VA_ARGS__); }


//--------------------------------------------------------------------------------------------------
/**
 * If the condition is true, print a generic message on stderr so that the user is aware there is a
 * problem, log the internal error message and exit.
 */
//--------------------------------------------------------------------------------------------------
#define INTERNAL_ERR_IF(condition, formatString, ...)                                   \
        if (condition) { INTERNAL_ERR(formatString, ##__VA_ARGS__); }


//--------------------------------------------------------------------------------------------------
/**
 * Error message for reading something in the remote process.
 */
//--------------------------------------------------------------------------------------------------
#define REMOTE_READ_ERR(x) "Error reading " #x " in the remote process."


//--------------------------------------------------------------------------------------------------
/**
 * Gets the counterpart address of the specified local reference in the address space of the
 * specified process.
 *
 * @return
 *      Remote address that is the counterpart of the local address.
 */
//--------------------------------------------------------------------------------------------------
static uintptr_t GetRemoteAddress
(
    pid_t pid,              ///< [IN] Remote process to to get the address for.
    void* localAddrPtr      ///< [IN] Local address to get the offset with.
)
{
    if (!LocalLibLegatoBaseAddr)
    {
        off_t localLibLegatoBaseAddrOff = 0;
        // Get the address of our framework library.
        if (addr_GetLibDataSection(0, "liblegato.so", &localLibLegatoBaseAddrOff) != LE_OK)
        {
            INTERNAL_ERR("Can't find our framework library address.");
        }

        LocalLibLegatoBaseAddr = localLibLegatoBaseAddrOff;
    }

    // Calculate the offset address of the local address by subtracting it by the start of our
    // own framwork library address.
    uintptr_t offset = (uintptr_t)(localAddrPtr) - LocalLibLegatoBaseAddr;

    if (!ChildLibLegatoBaseAddr)
    {
        off_t childLibLegatoBaseAddrOff = 0;

        // Get the address of the framework library in the remote process.
        if (addr_GetLibDataSection(pid, "liblegato.so", &childLibLegatoBaseAddrOff) != LE_OK)
        {
            INTERNAL_ERR("Can't find address of the framework library in the remote process.");
        }

        ChildLibLegatoBaseAddr = childLibLegatoBaseAddrOff;
    }

    // Calculate the process-under-inspection's counterpart address to the local address  by adding
    // the offset to the start of their framework library address.
    return (ChildLibLegatoBaseAddr + offset);
}

//--------------------------------------------------------------------------------------------------
/**
 * Attach to the target process in order to gain control of its execution and access its memory
 * space.
 */
//--------------------------------------------------------------------------------------------------
static void TargetAttach
(
    pid_t pid              ///< [IN] Remote process to attach to
)
{
    if (ptrace(PTRACE_SEIZE, pid, NULL, (void*)0) == -1)
    {
        fprintf(stderr, "Failed to attach to pid %d: error %d\n", pid, errno);
        LE_FATAL("Failed to attach to pid %d: error %d\n", pid, errno);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Detach from a process that we had previously attached to.
 */
//--------------------------------------------------------------------------------------------------
static void TargetDetach
(
    pid_t pid              ///< [IN] Remote process to detach from
)
{
    if (ptrace(PTRACE_DETACH, pid, 0, 0) == -1)
    {
        fprintf(stderr, "Failed to detach from pid %d: error %d\n", pid, errno);
        LE_FATAL("Failed to detach from pid %d: error %d\n", pid, errno);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Pause execution of a running process which we had previously attached to.
 */
//--------------------------------------------------------------------------------------------------
static void TargetStop
(
    pid_t pid              ///< [IN] Remote process to stop.
)
{
    int waitStatus;

    if (ptrace(PTRACE_INTERRUPT, pid, 0, 0) == -1)
    {
        fprintf(stderr, "Failed to stop pid %d: error %d\n", pid, errno);
        LE_FATAL("Failed to stop pid %d: error %d\n", pid, errno);
    }

    if (waitpid(pid, &waitStatus, 0) != pid)
    {
        fprintf(stderr, "Failed to wait for stopping pid %d: error %d\n", pid, errno);
        LE_FATAL("Failed to wait for stopping pid %d: error %d\n", pid, errno);
    }

    if (WIFEXITED(waitStatus))
    {
        fprintf(stderr, "Inspected process %d exited\n", pid);
        LE_FATAL("Inspected process %d exited\n", pid);
    }
    else if (WIFSTOPPED(waitStatus))
    {
        if (WSTOPSIG(waitStatus) != SIGTRAP && !PendingChildSignal)
        {
            // Stopped for a reason other than PTRACE interrupt (above) and no pending child
            // signal.  So store signal to be delivered later.
            PendingChildSignal = WSTOPSIG(waitStatus);
        }
    }
    else if (WIFSIGNALED(waitStatus))
    {
        // Store signal to pass along to the child when we restart
        if (!PendingChildSignal)
        {
            PendingChildSignal = WTERMSIG(PendingChildSignal);
        }
    }

    IsChildStopped = true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume execution of a previously paused process.
 */
//--------------------------------------------------------------------------------------------------
static void TargetStart
(
    pid_t pid              ///< [IN] Remote process to restart
)
{
    IsChildStopped = false;

    if (ptrace(PTRACE_CONT, pid, 0, (void *) (intptr_t) PendingChildSignal) == -1)
    {
        fprintf(stderr, "Failed to start pid %d: error %d\n", pid, errno);
        LE_FATAL("Failed to stop pid %d: error %d\n", pid, errno);
    }

    // Clear pending signal
    PendingChildSignal = 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read from the memory of an attached target process.
 */
//--------------------------------------------------------------------------------------------------
static bool TargetReadAddress
(
    pid_t pid,              ///< [IN] Remote process to read address
    uintptr_t remoteAddr,   ///< [IN] Remote address to read from target
    void* buffer,           ///< [OUT] Destination to read into
    size_t size             ///< [IN] Number of bytes to read
)
{
    LE_ASSERT(IsChildStopped);

    uintptr_t readWord;
    for (readWord = remoteAddr & ~(sizeof(long) - 1);
         size > 0;
         readWord += sizeof(long))
    {
        errno = 0;
        long peekWord = ptrace(PTRACE_PEEKDATA, pid, readWord, 0);

        // Check if ptrace was able to get memory
        if (errno != 0)
        {
            return LE_FAULT;
        }

        uintptr_t startOffset = (remoteAddr - readWord);
        size_t readSize = sizeof(long) - startOffset;
        LE_ASSERT(startOffset < sizeof(long));
        memcpy(buffer, ((char*)&peekWord) + startOffset, readSize);
        size -= readSize;
        remoteAddr += readSize;
        buffer = (char*)buffer + readSize;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize a RemoteDlsListAccess_t data struct.
 */
//--------------------------------------------------------------------------------------------------
static void InitRemoteDlsListAccessObj
(
    RemoteDlsListAccess_t* remoteList
)
{
    remoteList->List = LE_DLS_LIST_INIT;
    remoteList->ListChgCntRef = NULL;
    remoteList->headLinkPtr = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize a RemoteSlsListAccess_t data struct.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused))
static void InitRemoteSlsListAccessObj
(
    RemoteSlsListAccess_t* remoteList
)
{
    remoteList->List = LE_SLS_LIST_INIT;
    remoteList->ListChgCntRef = NULL;
    remoteList->headLinkPtr = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize a RemoteHashmapListAccess_t data struct.
 */
//--------------------------------------------------------------------------------------------------
static void InitRemoteHashmapListAccessObj
(
    RemoteHashmapListAccess_t* remoteList
)
{
#if LE_CONFIG_REDUCE_FOOTPRINT
        InitRemoteSlsListAccessObj(remoteList);
#else
        InitRemoteDlsListAccessObj(remoteList);
#endif
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an iterator that can be used to iterate over the list of available memory pools for a
 * specific process.
 *
 * @note
 *      The calling process must be root or have appropriate capabilities for this function and all
 *      subsequent operations on the iterator to succeed.
 *
 * @return
 *      An iterator to the list of memory pools for the specified process.
 */
//--------------------------------------------------------------------------------------------------
static MemPoolIter_Ref_t CreateMemPoolIter
(
    void
)
{
    // Get the address offset of the memory pool list for the process to inspect.
    uintptr_t listAddrOffset = GetRemoteAddress(PidToInspect, mem_GetPoolList());

    // Get the address offset of the memory pool list change counter for the process to inspect.
    uintptr_t listChgCntAddrOffset = GetRemoteAddress(PidToInspect, mem_GetPoolListChgCntRef());

    // Create the iterator.
    MemPoolIter_t* iteratorPtr = le_mem_ForceAlloc(IteratorPool);
    InitRemoteDlsListAccessObj(&iteratorPtr->memPoolList);

    // Get the List for the process-under-inspection.
    if (TargetReadAddress(PidToInspect, listAddrOffset, &(iteratorPtr->memPoolList.List),
                             sizeof(iteratorPtr->memPoolList.List)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("mempool list"));
    }

    // Get the ListChgCntRef for the process-under-inspection.
    if (TargetReadAddress(PidToInspect, listChgCntAddrOffset,
                          &(iteratorPtr->memPoolList.ListChgCntRef),
                          sizeof(iteratorPtr->memPoolList.ListChgCntRef)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("mempool list change counter ref"));
    }

    return iteratorPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an iterator that can be used to iterate over the list of thread objects for a specific
 * process. See the comment block for CreateMemPoolIter for additional detail.
 *
 * @return
 *      An iterator to the list of thread objects for the specified process.
 */
//--------------------------------------------------------------------------------------------------
static ThreadObjIter_Ref_t CreateThreadObjIter
(
    void
)
{
    // Get the address offset of the thread obj list for the process to inspect.
    uintptr_t listAddrOffset = GetRemoteAddress(PidToInspect, thread_GetThreadObjList());

    // Get the address offset of the list of thread objs change counter for the process to inspect.
    uintptr_t listChgCntAddrOffset = GetRemoteAddress(PidToInspect, thread_GetThreadObjListChgCntRef());

    // Create the iterator.
    ThreadObjIter_t* iteratorPtr = le_mem_ForceAlloc(IteratorPool);
    InitRemoteDlsListAccessObj(&iteratorPtr->threadObjList);

    // Get the List for the process-under-inspection.
    if (TargetReadAddress(PidToInspect, listAddrOffset, &(iteratorPtr->threadObjList.List),
                          sizeof(iteratorPtr->threadObjList.List)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread obj list"));
    }

    // Get the ListChgCntRef for the process-under-inspection.
    if (TargetReadAddress(PidToInspect, listChgCntAddrOffset,
                          &(iteratorPtr->threadObjList.ListChgCntRef),
                          sizeof(iteratorPtr->threadObjList.ListChgCntRef)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread obj list change counter ref"));
    }

    return iteratorPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an iterator that can be used to iterate over the list of thread member objects for a
 * specific process.
 * See the comment block for CreateMemPoolIter for additional detail.
 *
 * @return
 *      An iterator to the list of thread member objects for the specified process.
 */
//--------------------------------------------------------------------------------------------------
static void* CreateThreadMemberObjIter
(
    InspType_t memberObjType ///< [IN] The type of iterator to create.
)
{
    // function prototype for the module_GetXXXChgCntRef family.
    size_t** (*getListChgCntRefFunc)(void);

    switch (memberObjType)
    {
        case INSPECT_INSP_TYPE_TIMER:
            getListChgCntRefFunc = timer_GetTimerListChgCntRef;
            break;

        case INSPECT_INSP_TYPE_MUTEX:
            getListChgCntRefFunc = mutex_GetMutexListChgCntRef;
            break;

        case INSPECT_INSP_TYPE_SEMAPHORE:
            getListChgCntRefFunc = sem_GetSemaphoreListChgCntRef;
            break;

        default:
            INTERNAL_ERR("unexpected thread member object type %d.", memberObjType);
    }

    // Get the address offset of the list of thread objs for the process to inspect.
    uintptr_t threadObjListAddrOffset = GetRemoteAddress(PidToInspect, thread_GetThreadObjList());

    // Get the addr offset of the change counter of the list of thread objs for the process to
    // inspect.
    uintptr_t threadObjListChgCntAddrOffset = GetRemoteAddress(PidToInspect,
                                                           thread_GetThreadObjListChgCntRef());

    // Get the address offset of the change counter of the list of thread member objs for the
    // process to inspect.
    uintptr_t threadMemberObjListChgCntAddrOffset = GetRemoteAddress(PidToInspect,
                                                                 getListChgCntRefFunc());

    // Create the iterator.
    ThreadMemberObjIter_t* iteratorPtr = le_mem_ForceAlloc(IteratorPool);
    InitRemoteDlsListAccessObj(&iteratorPtr->threadObjList);
    InitRemoteDlsListAccessObj(&iteratorPtr->threadMemberObjList);

    // Get the list of thread objs for the process-under-inspection.
    if (TargetReadAddress(PidToInspect, threadObjListAddrOffset, &(iteratorPtr->threadObjList.List),
                             sizeof(iteratorPtr->threadObjList.List)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread obj list"));
    }

    // Get the thread obj ListChgCntRef for the process-under-inspection.
    if (TargetReadAddress(PidToInspect, threadObjListChgCntAddrOffset,
                          &(iteratorPtr->threadObjList.ListChgCntRef),
                          sizeof(iteratorPtr->threadObjList.ListChgCntRef)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread obj list change counter ref"));
    }

    // Get the thread member obj ListChgCntRef for the process-under-inspection.
    if (TargetReadAddress(PidToInspect, threadMemberObjListChgCntAddrOffset,
                          &(iteratorPtr->threadMemberObjList.ListChgCntRef),
                          sizeof(iteratorPtr->threadMemberObjList.ListChgCntRef)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread member obj list change counter ref"));
    }

    return iteratorPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an iterator that can be used to iterate over the list of timers, mutexes, or semaphores
 * for a specific process.  These are wrappers for CreateThreadMemberObjIter.
 * See the comment block for CreateMemPoolIter for additional detail.
 *
 * @return
 *      An iterator to the list of timers/mutexes/semaphores for the specified process.
 */
//--------------------------------------------------------------------------------------------------
static TimerIter_Ref_t CreateTimerIter
(
    void
)
{
    return (TimerIter_Ref_t)CreateThreadMemberObjIter(INSPECT_INSP_TYPE_TIMER);
}

static MutexIter_Ref_t CreateMutexIter
(
    void
)
{
    return (MutexIter_Ref_t)CreateThreadMemberObjIter(INSPECT_INSP_TYPE_MUTEX);
}

static SemaphoreIter_Ref_t CreateSemaphoreIter
(
    void
)
{
    return (SemaphoreIter_Ref_t)CreateThreadMemberObjIter(INSPECT_INSP_TYPE_SEMAPHORE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an iterator that can be used to iterate over the map of interface objects. See the
 * comment block for CreateMemPoolIter for additional detail.
 *
 * @return
 *      An iterator to the map of interface objects.
 */
//--------------------------------------------------------------------------------------------------
static InterfaceObjIter_Ref_t CreateInterfaceObjIter
(
    InspType_t interfaceType ///< [IN] The type of iterator to create.
)
{
    // function prototype for the module_GetXXXMapChgCntRef family.
    size_t** (*getMapChgCntRefFunc)(void);
    // function prototype for the module_GetXXXMap family.
    le_hashmap_Ref_t* (*getMapFunc)(void);

    switch (interfaceType)
    {
        case INSPECT_INSP_TYPE_IPC_SERVERS:
            getMapChgCntRefFunc = msgInterface_GetServiceObjMapChgCntRef;
            getMapFunc          = msgInterface_GetServiceObjMap;
            break;

        case INSPECT_INSP_TYPE_IPC_CLIENTS:
            getMapChgCntRefFunc = msgInterface_GetClientInterfaceMapChgCntRef;
            getMapFunc          = msgInterface_GetClientInterfaceMap;
            break;

        case INSPECT_INSP_TYPE_IPC_SERVERS_SESSIONS:
            getMapChgCntRefFunc = msgInterface_GetServiceObjMapChgCntRef;
            getMapFunc          = msgInterface_GetServiceObjMap;
            break;

        case INSPECT_INSP_TYPE_IPC_CLIENTS_SESSIONS:
            getMapChgCntRefFunc = msgInterface_GetClientInterfaceMapChgCntRef;
            getMapFunc          = msgInterface_GetClientInterfaceMap;
            break;

        default:
            INTERNAL_ERR("unexpected interface object type %d.", interfaceType);
    }


    // Get the address offset of the map of interface objs for the process to inspect.
    uintptr_t mapAddrOffset = GetRemoteAddress(PidToInspect, getMapFunc());

    // Get the address offset of the map of interface objs change counter for the proc to inspect.
    uintptr_t mapChgCntAddrOffset = GetRemoteAddress(PidToInspect, getMapChgCntRefFunc());

    // Create the iterator.
    InterfaceObjIter_t* iteratorPtr = le_mem_ForceAlloc(IteratorPool);

    le_hashmap_Ref_t mapRef;
    le_hashmap_Hashmap_t map;

    // Get the mapRef for the process-under-inspection.
    if (TargetReadAddress(PidToInspect, mapAddrOffset, &(mapRef), sizeof(mapRef)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("interface obj map ref"));
    }

    // Get the map for the process-under-inspection.
    if (TargetReadAddress(PidToInspect, (uintptr_t)mapRef, &(map), sizeof(map)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("interface obj map"));
    }

    iteratorPtr->interfaceObjMap.bucketsPtr = map.bucketsPtr;
    iteratorPtr->interfaceObjMap.bucketCount = map.bucketCount;

    // Get the mapChgCntRef for the process-under-inspection.
    if (TargetReadAddress(PidToInspect, mapChgCntAddrOffset,
                          &(iteratorPtr->interfaceObjMap.mapChgCntRef),
                          sizeof(iteratorPtr->interfaceObjMap.mapChgCntRef)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("interface obj map change counter ref"));
    }

    // Initialization.
    iteratorPtr->currIndex = 0;

    InitRemoteHashmapListAccessObj(&iteratorPtr->interfaceObjList);

    // Get the list of interface objects.
    if (TargetReadAddress(PidToInspect, (uintptr_t)iteratorPtr->interfaceObjMap.bucketsPtr,
                          &(iteratorPtr->interfaceObjList.List),
                          sizeof(iteratorPtr->interfaceObjList.List)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("interface obj list of bucket 0 in the interface obj map"));
    }

    return iteratorPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an iterator that can be used to iterate over the map of service objects for a specific
 * process. See the comment block for CreateMemPoolIter for additional detail.
 *
 * @return
 *      An iterator to the map of service objects.
 */
//--------------------------------------------------------------------------------------------------
static ServiceObjIter_Ref_t CreateServiceObjIter
(
    void
)
{
    return ((ServiceObjIter_Ref_t)CreateInterfaceObjIter(INSPECT_INSP_TYPE_IPC_SERVERS));
}

static ClientObjIter_Ref_t CreateClientObjIter
(
    void
)
{
    return ((ClientObjIter_Ref_t)CreateInterfaceObjIter(INSPECT_INSP_TYPE_IPC_CLIENTS));
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an iterator that can be used to iterate over the list of session objects for a specific
 * process. See the comment block for CreateMemPoolIter for additional detail.
 *
 * @return
 *      An iterator to the list of session objects.
 */
//--------------------------------------------------------------------------------------------------
static SessionObjIter_Ref_t CreateSessionObjIter
(
    void
)
{
    SessionObjIter_Ref_t iteratorPtr;

    switch (InspectType)
    {
        case INSPECT_INSP_TYPE_IPC_SERVERS_SESSIONS:
            iteratorPtr = (SessionObjIter_Ref_t)CreateInterfaceObjIter(
                            INSPECT_INSP_TYPE_IPC_SERVERS_SESSIONS);
            break;

        case INSPECT_INSP_TYPE_IPC_CLIENTS_SESSIONS:
            iteratorPtr = (SessionObjIter_Ref_t)CreateInterfaceObjIter(
                            INSPECT_INSP_TYPE_IPC_CLIENTS_SESSIONS);
            break;

        default:
            INTERNAL_ERR("unexpected inspect type %d.", InspectType);
    }

    // Get the address offset of the list of session objs change counter for the proc to inspect.
    uintptr_t listChgCntAddrOffset = GetRemoteAddress(PidToInspect,
                                                  msgSession_GetSessionObjListChgCntRef());

    // Initialize the list.
    InitRemoteDlsListAccessObj(&iteratorPtr->sessionList);

    // Get the listChgCntRef for the process-under-inspection.
    if (TargetReadAddress(PidToInspect, listChgCntAddrOffset,
                          &(iteratorPtr->sessionList.ListChgCntRef),
                          sizeof(iteratorPtr->sessionList.ListChgCntRef)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("session obj list change counter ref"));
    }

    return iteratorPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the memory pool list change counter from the specified iterator.
 *
 * @return
 *      List change counter.
 */
//--------------------------------------------------------------------------------------------------
static size_t GetMemPoolListChgCnt
(
    MemPoolIter_Ref_t iterator ///< [IN] The iterator to get the list change counter from.
)
{
    size_t memPoolListChgCnt;
    if (TargetReadAddress(PidToInspect, (uintptr_t)(iterator->memPoolList.ListChgCntRef),
                          &memPoolListChgCnt, sizeof(memPoolListChgCnt)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("mempool list change counter"));
    }

    return memPoolListChgCnt;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the thread object list change counter from the specified iterator.
 *
 * @return
 *      List change counter.
 */
//--------------------------------------------------------------------------------------------------
static size_t GetThreadObjListChgCnt
(
    ThreadObjIter_Ref_t iterator ///< [IN] The iterator to get the list change counter from.
)
{
    size_t threadObjListChgCnt;
    if (TargetReadAddress(PidToInspect, (uintptr_t)(iterator->threadObjList.ListChgCntRef),
                          &threadObjListChgCnt, sizeof(threadObjListChgCnt)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread obj list change counter"));
    }

    return threadObjListChgCnt;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the timer list change counter from the specified iterator. Note while there's one timer list
 * for a thread, the timer list change counter is for all timer lists. Also the timer list is
 * considered "changed" if the thread object list has changed (hence the addition of the timer and
 * thread object list change counters). The same applies to the mutex and semaphore list change
 * counter.
 *
 * @return
 *      List change counter.
 */
//--------------------------------------------------------------------------------------------------
static size_t GetThreadMemberObjListChgCnt
(
    ThreadMemberObjIter_Ref_t iterator ///< [IN] The iterator to get the list change counter from.
)
{
    size_t threadObjListChgCnt, threadMemberObjListChgCnt;
    if (TargetReadAddress(PidToInspect, (uintptr_t)(iterator->threadObjList.ListChgCntRef),
                          &threadObjListChgCnt, sizeof(threadObjListChgCnt)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread obj list change counter"));
    }

    if (TargetReadAddress(PidToInspect, (uintptr_t)(iterator->threadMemberObjList.ListChgCntRef),
                          &threadMemberObjListChgCnt, sizeof(threadMemberObjListChgCnt)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread member obj list change counter"));
    }

    return (threadObjListChgCnt + threadMemberObjListChgCnt);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the interface object map change counter from the specified iterator.
 *
 * @return
 *      Map change counter.
 */
//--------------------------------------------------------------------------------------------------
static size_t GetInterfaceObjMapChgCnt
(
    InterfaceObjIter_Ref_t iterator ///< [IN] The iterator to get the map change counter from.
)
{
    size_t interfaceObjMapChgCnt;
    if (TargetReadAddress(PidToInspect, (uintptr_t)(iterator->interfaceObjMap.mapChgCntRef),
                          &interfaceObjMapChgCnt, sizeof(interfaceObjMapChgCnt)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("interface obj map change counter"));
    }

    return interfaceObjMapChgCnt;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the session list change counter from the specified iterator. The session list is also
 * considered "changed" if the interface object has changed.
 *
 * @return
 *      Map change counter.
 */
//--------------------------------------------------------------------------------------------------
static size_t GetSessionListChgCnt
(
    SessionObjIter_Ref_t iterator ///< [IN] The iterator to get the list change counter from.
)
{
    size_t sessionListChgCnt;
    if (TargetReadAddress(PidToInspect, (uintptr_t)(iterator->sessionList.ListChgCntRef),
                          &sessionListChgCnt, sizeof(sessionListChgCnt)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("session list change counter"));
    }

    return GetInterfaceObjMapChgCnt((InterfaceObjIter_Ref_t)iterator) + sessionListChgCnt;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next link of the provided link. This is for accessing a list in a remote process,
 * otherwise the doubly linked list API can simply be used. Note that "linkRef" is a ref to a
 * locally existing link obj, which is a link for a remote node. Therefore GetNextDlsLink cannot be
 * called back-to-back.
 *
 * Also, if GetNextDlsLink is called the first time for a given listInfoRef, linkRef is not used.
 *
 * After calling GetNextDlsLink, the returned link ptr must be used to read the associated remote
 * node into the local memory space. One would then retrieve the link object from the node, and then
 * GetNextDlsLink can be called on the ref of that link object.
 *
 * @return
 *      Pointer to a link of a node in the remote process
 */
//--------------------------------------------------------------------------------------------------
static le_dls_Link_t* GetNextDlsLink
(
    RemoteDlsListAccess_t* listInfoRef,    ///< [IN] Object for accessing a list in the remote process.
    le_dls_Link_t* linkRef              ///< [IN] Link of a node in the remote process. This is a
                                        ///<      ref to a locally existing link obj.
)
{
    INTERNAL_ERR_IF(listInfoRef == NULL,
                    "obj ref for accessing a list in the remote process is NULL.");

    // Create a fake list of nodes that has a single element.  Use this when iterating over the
    // links in the list because the links read from the mems file is in the address space of the
    // process under test.  Using a fake list guarantees that the linked list operation does not
    // accidentally reference memory in our own memory space.  This means that we have to check
    // for the end of the list manually.
    le_dls_List_t fakeList = LE_DLS_LIST_INIT;
    le_dls_Link_t fakeLink = LE_DLS_LINK_INIT;
    le_dls_Stack(&fakeList, &fakeLink);

    // Get the next link in the list.
    le_dls_Link_t* LinkPtr;

    if (listInfoRef->headLinkPtr == NULL)
    {
        // Get the address of the first node's link.
        LinkPtr = le_dls_Peek(&(listInfoRef->List));

        // The list is empty
        if (LinkPtr == NULL)
        {
            return NULL;
        }

        listInfoRef->headLinkPtr = LinkPtr;
    }
    else
    {
        // Get the address of the next node.
        LinkPtr = le_dls_PeekNext(&fakeList, linkRef);

        if (LinkPtr == listInfoRef->headLinkPtr)
        {
            // Looped back to the first node so there are no more nodes.
            return NULL;
        }
    }

    return LinkPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next link of the provided link. This is for accessing a list in a remote process,
 * otherwise the doubly linked list API can simply be used. Note that "linkRef" is a ref to a
 * locally existing link obj, which is a link for a remote node. Therefore GetNextSlsLink cannot be
 * called back-to-back.
 *
 * Also, if GetNextSlsLink is called the first time for a given listInfoRef, linkRef is not used.
 *
 * After calling GetNextSlsLink, the returned link ptr must be used to read the associated remote
 * node into the local memory space. One would then retrieve the link object from the node, and then
 * GetNextSlsLink can be called on the ref of that link object.
 *
 * @return
 *      Pointer to a link of a node in the remote process
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused))
static le_sls_Link_t* GetNextSlsLink
(
    RemoteSlsListAccess_t* listInfoRef,    ///< [IN] Object for accessing a list in the remote process.
    le_sls_Link_t* linkRef              ///< [IN] Link of a node in the remote process. This is a
                                        ///<      ref to a locally existing link obj.
)
{
    INTERNAL_ERR_IF(listInfoRef == NULL,
                    "obj ref for accessing a list in the remote process is NULL.");

    // Create a fake list of nodes that has a single element.  Use this when iterating over the
    // links in the list because the links read from the mems file is in the address space of the
    // process under test.  Using a fake list guarantees that the linked list operation does not
    // accidentally reference memory in our own memory space.  This means that we have to check
    // for the end of the list manually.
    le_sls_List_t fakeList = LE_SLS_LIST_INIT;
    le_sls_Link_t fakeLink = LE_SLS_LINK_INIT;
    le_sls_Stack(&fakeList, &fakeLink);

    // Get the next link in the list.
    le_sls_Link_t* LinkPtr;

    if (listInfoRef->headLinkPtr == NULL)
    {
        // Get the address of the first node's link.
        LinkPtr = le_sls_Peek(&(listInfoRef->List));

        // The list is empty
        if (LinkPtr == NULL)
        {
            return NULL;
        }

        listInfoRef->headLinkPtr = LinkPtr;
    }
    else
    {
        // Get the address of the next node.
        LinkPtr = le_sls_PeekNext(&fakeList, linkRef);

        if (LinkPtr == listInfoRef->headLinkPtr)
        {
            // Looped back to the first node so there are no more nodes.
            return NULL;
        }
    }

    return LinkPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next link of the provided link. This is for accessing a list in a remote process,
 * otherwise the doubly linked list API can simply be used. Note that "linkRef" is a ref to a
 * locally existing link obj, which is a link for a remote node. Therefore GetNextHashmapLink cannot be
 * called back-to-back.
 *
 * Also, if GetNextHashmapLink is called the first time for a given listInfoRef, linkRef is not used.
 *
 * After calling GetNextHashmapLink, the returned link ptr must be used to read the associated remote
 * node into the local memory space. One would then retrieve the link object from the node, and then
 * GetNextHashmapLink can be called on the ref of that link object.
 *
 * @return
 *      Pointer to a link of a node in the remote process
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Link_t* GetNextHashmapLink
(
    RemoteHashmapListAccess_t* listInfoRef,    ///< [IN] Object for accessing a list in the remote process.
    le_hashmap_Link_t* linkRef      ///< [IN] Link of a node in the remote process. This is a
                                    ///<      ref to a locally existing link obj.
)
{
#if LE_CONFIG_REDUCE_FOOTPRINT
    return GetNextSlsLink(listInfoRef, linkRef);
#else
    return GetNextDlsLink(listInfoRef, linkRef);
#endif
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next memory pool from the specified iterator.  The first time this function is called
 * after CreateMemPoolIter() is called, the first memory pool in the list is returned.  The second
 * time this function is called the second memory pool is returned and so on.
 *
 * @warning
 *      The memory pool returned by this function belongs to the remote process.  Do not attempt to
 *      expand the pool or allocate objects from the pool, doing so will lead to memory leaks in
 *      the calling process.
 *
 * @return
 *      A memory pool from the iterator's list of memory pools.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_Pool_t* GetNextMemPool
(
    MemPoolIter_Ref_t memPoolIterRef ///< [IN] The iterator to get the next mem pool from.
)
{
    le_dls_Link_t* linkPtr = GetNextDlsLink(&(memPoolIterRef->memPoolList),
                                         &(memPoolIterRef->currMemPool.poolLink));

    if (linkPtr == NULL)
    {
        return NULL;
    }

    // Get the address of pool.
    le_mem_Pool_t* poolPtr = CONTAINER_OF(linkPtr, le_mem_Pool_t, poolLink);

    // Read the pool into our own memory.
    if (TargetReadAddress(PidToInspect, (uintptr_t)poolPtr, &(memPoolIterRef->currMemPool),
                          sizeof(memPoolIterRef->currMemPool)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("mempool object"));
    }

    return &(memPoolIterRef->currMemPool);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next thread object from the specified iterator. For other detail see GetNextMemPool.
 *
 * @return
 *      A thread object from the iterator's list of thread objects.
 */
//--------------------------------------------------------------------------------------------------
static thread_Obj_t* GetNextThreadObj
(
    ThreadObjIter_Ref_t threadObjIterRef ///< [IN] The iterator to get the next thread obj from.
)
{
    le_dls_Link_t* linkPtr = GetNextDlsLink(&(threadObjIterRef->threadObjList),
                                         &(threadObjIterRef->currThreadObj.link));

    if (linkPtr == NULL)
    {
        return NULL;
    }

    // Get the address of thread obj.
    thread_Obj_t* threadObjPtr = CONTAINER_OF(linkPtr, thread_Obj_t, link);

    // Read the thread obj into our own memory.
    if (TargetReadAddress(PidToInspect, (uintptr_t)threadObjPtr, &(threadObjIterRef->currThreadObj),
                          sizeof(threadObjIterRef->currThreadObj)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread object"));
    }

    return &(threadObjIterRef->currThreadObj);
}


//--------------------------------------------------------------------------------------------------
/**
 * Given a thread object, retrieve the thread member object list based on the member type specified.
 *
 * @return
 *      The thread member object list under inspection.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_Link_t* GetThreadMemberObjList
(
    InspType_t memberObjType, ///< [IN] The type of the thread member object.
    thread_Obj_t* threadObjRef         ///< [IN] Thread object ref.
)
{
    switch (memberObjType)
    {
        case INSPECT_INSP_TYPE_TIMER:
            return threadObjRef->timerRecPtr[TIMER_NON_WAKEUP]->activeTimerList.headLinkPtr;
            break;

        case INSPECT_INSP_TYPE_MUTEX:
            return threadObjRef->mutexRec.lockedMutexList.headLinkPtr;
            break;

        default:
            INTERNAL_ERR("unexpected thread member object type %d.", memberObjType);
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next thread member object link ptr from the specified iterator. For other detail see
 * GetNextMemPool. This is a helper function for GetNextTimer and GetNextMutex.
 *
 * @return
 *      A thread member obj link ptr.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_Link_t* GetNextThreadMemberObjLinkPtr
(
    InspType_t memberObjType,                       ///< [IN] The type of thread member object.
    ThreadMemberObjIter_Ref_t threadMemberObjItrRef ///< [IN] The iterator to get the next thread
                                                    ///< member obj from.
)
{
    le_dls_Link_t* currThreadMemberObjLinkPtr;

    switch (memberObjType)
    {
        case INSPECT_INSP_TYPE_TIMER:
            // an empty statement for the label to belong to, since a declaration is not a statement
            // in C.
            ;
            TimerIter_Ref_t timerIterRef = (TimerIter_Ref_t)threadMemberObjItrRef;
            currThreadMemberObjLinkPtr = &(timerIterRef->currTimer.link);
            break;

        case INSPECT_INSP_TYPE_MUTEX:
            ;
            MutexIter_Ref_t mutexIterRef = (MutexIter_Ref_t)threadMemberObjItrRef;
            currThreadMemberObjLinkPtr = &(mutexIterRef->currMutex.lockedByThreadLink);
            break;

        default:
            INTERNAL_ERR("unexpected thread member object type %d.", memberObjType);
    }

    // local references to the timer and thread objects.
    thread_Obj_t* localThreadObjRef;
    le_dls_Link_t* remThreadObjNextLinkPtr;
    le_dls_Link_t* remThreadMemberObjNextLinkPtr;

    // Get the next thread member obj
    remThreadMemberObjNextLinkPtr = GetNextDlsLink(&(threadMemberObjItrRef->threadMemberObjList),
                                                currThreadMemberObjLinkPtr);
    while (remThreadMemberObjNextLinkPtr == NULL)
    {
        remThreadObjNextLinkPtr = GetNextDlsLink(&(threadMemberObjItrRef->threadObjList),
                                              &(threadMemberObjItrRef->currThreadObj.link));

        // There are no more thread objects on the list (or list is empty)
        if (remThreadObjNextLinkPtr == NULL)
        {
            return NULL;
        }

        // Get the address of thread obj.
        thread_Obj_t* remThreadObjPtr = CONTAINER_OF(remThreadObjNextLinkPtr, thread_Obj_t, link);

        // Read the thread obj into our own memory, and update the local reference
        if (TargetReadAddress(PidToInspect, (uintptr_t)remThreadObjPtr,
                              &(threadMemberObjItrRef->currThreadObj),
                              sizeof(threadMemberObjItrRef->currThreadObj)) != LE_OK)
        {
            INTERNAL_ERR(REMOTE_READ_ERR("thread object"));
        }
        localThreadObjRef = &(threadMemberObjItrRef->currThreadObj);

        // retrieve the thread member obj list for the thread object; update our thread member obj
        // list with that list, and reset our local copy of the thread member obj list head.
        threadMemberObjItrRef->threadMemberObjList.List.headLinkPtr =
             GetThreadMemberObjList(memberObjType, localThreadObjRef);
        threadMemberObjItrRef->threadMemberObjList.headLinkPtr = NULL;

        // Get the next thread member obj.
        remThreadMemberObjNextLinkPtr = GetNextDlsLink(&(threadMemberObjItrRef->threadMemberObjList),
                                                    NULL);
    }

    return remThreadMemberObjNextLinkPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next timer from the specified iterator. All timers from all thread objects are
 * considered to be on a single timer list. Therefore the out param would be NULL only when all
 * timer lists from all thread objects have been iterated.
 *
 * @return
 *      A timer from the iterator's list of timers.
 */
//--------------------------------------------------------------------------------------------------
static Timer_t* GetNextTimer
(
    TimerIter_Ref_t timerIterRef ///< [IN] The iterator to get the next timer from.
)
{
    le_dls_Link_t* remThreadMemberObjNextLinkPtr =
        GetNextThreadMemberObjLinkPtr(INSPECT_INSP_TYPE_TIMER,
                                      (ThreadMemberObjIter_Ref_t)timerIterRef);

    if (remThreadMemberObjNextLinkPtr == NULL)
    {
        return NULL;
    }

    // Get the address of timer.
    Timer_t* remTimerPtr = CONTAINER_OF(remThreadMemberObjNextLinkPtr, Timer_t, link);

    // Read the timer into our own memory.
    if (TargetReadAddress(PidToInspect, (uintptr_t)remTimerPtr, &(timerIterRef->currTimer),
                          sizeof(timerIterRef->currTimer)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("timer object"));
    }

    return &(timerIterRef->currTimer);
}


//--------------------------------------------------------------------------------------------------
/**
 * See GetNextTimer.
 *
 * @return
 *      A mutex from the iterator's list of mutexes.
 */
//--------------------------------------------------------------------------------------------------
static Mutex_t* GetNextMutex
(
    MutexIter_Ref_t mutexIterRef ///< [IN] The iterator to get the next mutex from.
)
{
    le_dls_Link_t* remThreadMemberObjNextLinkPtr =
        GetNextThreadMemberObjLinkPtr(INSPECT_INSP_TYPE_MUTEX,
                                      (ThreadMemberObjIter_Ref_t)mutexIterRef);

    if (remThreadMemberObjNextLinkPtr == NULL)
    {
        return NULL;
    }

    // Get the address of mutex.
    Mutex_t* remMutexPtr = CONTAINER_OF(remThreadMemberObjNextLinkPtr, Mutex_t, lockedByThreadLink);

    // Read the mutex into our own memory.
    if (TargetReadAddress(PidToInspect, (uintptr_t)remMutexPtr, &(mutexIterRef->currMutex),
                          sizeof(mutexIterRef->currMutex)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("mutex object"));
    }

    return &(mutexIterRef->currMutex);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the next semaphore. Since there's no "semaphore list" and therefore each thread object owns
 * one semaphore object directly (ie. not on a list), this is handled a little differently from
 * other GetNextXXX fcns. This takes advantage of the existing GetNextThreadObj, updates the
 * "current thread obj" in the iterator, and access the semaphore object directly. Note that this
 * fcn could've been done the same way as GetNextMutex and Timer since semaphore is also a "thread
 * member obj", but I've decided to do it this way instead of creating dummies and jumping through
 * hoops to satisfy the abstraction.
 *
 * @return
 *      A semaphore from the iterator's list of thread obj.
 */
//--------------------------------------------------------------------------------------------------
static Semaphore_t* GetNextSemaphore
(
    SemaphoreIter_Ref_t semaIterRef ///< [IN] The iterator to get the next sempaphore from.
)
{
    Semaphore_t* remSemaphorePtr;
    thread_Obj_t* currThreadObjRef;

    // Create a local thread obj iterator based on the semaphore iterator that's passed in.
    ThreadObjIter_t threadObjIter;
    threadObjIter.threadObjList = semaIterRef->threadObjList;
    threadObjIter.currThreadObj = semaIterRef->currThreadObj;

    // Access the next thread obj directly since there's no "semaphore list" and each thread obj
    // owns at most one semaphore obj, in contrast of the logic of GetNextThreadMemberObjLinkPtr.
    do
    {
        // Get the next thread obj based on the semaphore iterator.
        currThreadObjRef = GetNextThreadObj(&threadObjIter);

        // Update the "current" thread object in the semaphore iterator.
        semaIterRef->currThreadObj = threadObjIter.currThreadObj;
        // Also need to update the list (or rather the headLinkPtr in it). Otherwise next time
        // GetNextSemaphore is called, GetNextThreadObj still returns the "first" thread obj.
        semaIterRef->threadObjList = threadObjIter.threadObjList;

        // No more thread objects, and therefore no more semaphore objects.
        if (currThreadObjRef == NULL)
        {
            return NULL;
        }

        // Get the address of semaphore.
        remSemaphorePtr = currThreadObjRef->semaphoreRec.waitingOnSemaphore;
    }
    while (remSemaphorePtr == NULL);

    // Read the semaphore into our own memory.
    if (TargetReadAddress(PidToInspect, (uintptr_t)remSemaphorePtr, &(semaIterRef->currSemaphore),
                          sizeof(semaIterRef->currSemaphore)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("semaphore object"));
    }

    return &(semaIterRef->currSemaphore);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the pointer to the next interface instance object. For other detail see GetNextMemPool.
 *
 * @return
 *      A pointer to an interface instace object.
 */
//--------------------------------------------------------------------------------------------------
static void* GetNextInterfaceObjPtr
(
    InterfaceObjIter_Ref_t iterator ///< [IN] The iterator to get the next interface obj from.
)
{
    // with iterator->currIndex and iterator->interfaceObjList initialized in the createIterator
    // fcn, GetNextDlsLink on interfaceObjList.  If the returned link is null, then upadte currIndex
    // and interfaceObjList.

    le_hashmap_Link_t* remEntryNextLinkPtr;

    // Get the link of the next item on the interface object list.
    remEntryNextLinkPtr = GetNextHashmapLink(&(iterator->interfaceObjList),
                                             &(iterator->currEntry.entryListLink));

    // If the link is null, then update our list by accessing the next bucket, and attempt to
    // Get the link from the updated list.
    while (remEntryNextLinkPtr == NULL)
    {
        // Increment the bucket index. Return null if we run out of buckets.
        if (iterator->currIndex < (iterator->interfaceObjMap.bucketCount - 1))
        {
            iterator->currIndex++;
        }
        else
        {
            return NULL;
        }

        // So we haven't run out of buckets yet. Then update our interface object list.
        if (TargetReadAddress(PidToInspect,
                              (uintptr_t)(iterator->interfaceObjMap.bucketsPtr + iterator->currIndex),
                              &(iterator->interfaceObjList.List),
                              sizeof(iterator->interfaceObjList.List)) != LE_OK)
        {
            INTERNAL_ERR(REMOTE_READ_ERR("interface obj list of bucket %zu in the interface"
                                          "obj map"), iterator->currIndex);
        }

        // With the updated interface obj list, also set the head link ptr null.
        iterator->interfaceObjList.headLinkPtr = NULL;

        // With the updated interface object list, get the link of the next item.
        remEntryNextLinkPtr = GetNextHashmapLink(&(iterator->interfaceObjList), NULL);
    }

    // The node that the link belongs to is technically le_hashmap_Entry_t which contains a ptr to an
    // interface instace obj (server, client, etc.)
    le_hashmap_Entry_t* remEntryPtr = CONTAINER_OF(remEntryNextLinkPtr, le_hashmap_Entry_t, entryListLink);

    // Read the entry object into our own memory.
    if (TargetReadAddress(PidToInspect, (uintptr_t)remEntryPtr,
                          &(iterator->currEntry), sizeof(iterator->currEntry)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("entry object"));
    }

    return (void*)iterator->currEntry.valuePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next service object from the specified iterator. For other detail see GetNextMemPool.
 *
 * @return
 *      A service object from the iterator's list of service objects.
 */
//--------------------------------------------------------------------------------------------------
static msgInterface_UnixService_t* GetNextServiceObj
(
    ServiceObjIter_Ref_t serviceObjIterRef ///< [IN] The iterator to get the next service obj from.
)
{
    // Gets the pointer of the next service object.
    msgInterface_UnixService_t* serviceObjPtr
        = GetNextInterfaceObjPtr((InterfaceObjIter_Ref_t)serviceObjIterRef);

    if (serviceObjPtr == NULL)
    {
        return NULL;
    }

    // Read the service object into our own memory.
    if (TargetReadAddress(PidToInspect, (uintptr_t)serviceObjPtr, &(serviceObjIterRef->currServiceObj),
                          sizeof(serviceObjIterRef->currServiceObj)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("service object"));
    }

    return &(serviceObjIterRef->currServiceObj);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next client interface object from the specified iterator. For other detail see
 * GetNextMemPool.
 *
 * @return
 *      A client interface object from the iterator's list of client interface objects.
 */
//--------------------------------------------------------------------------------------------------
static msgInterface_ClientInterface_t* GetNextClientObj
(
    ClientObjIter_Ref_t clientObjIterRef
                      ///< [IN] The iterator to get the next client interface obj from.
)
{
    // Gets the pointer of the next service object.
    msgInterface_ClientInterface_t* clientObjPtr
        = GetNextInterfaceObjPtr((InterfaceObjIter_Ref_t)clientObjIterRef);

    if (clientObjPtr == NULL)
    {
        return NULL;
    }

    // Read the client interface object into our own memory.
    if (TargetReadAddress(PidToInspect, (uintptr_t)clientObjPtr, &(clientObjIterRef->currClientObj),
                          sizeof(clientObjIterRef->currClientObj)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("client interface object"));
    }

    return &(clientObjIterRef->currClientObj);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next session object from the specified iterator. For other detail see GetNextMemPool.
 *
 * @return
 *      A session object from the iterator's list of session objects.
 */
//--------------------------------------------------------------------------------------------------
static msgSession_UnixSession_t* GetNextSessionObj
(
    SessionObjIter_Ref_t sessionObjIterRef ///< [IN] The iterator to get the next session obj from.
)
{
    le_dls_Link_t* remSessionNextLinkPtr;
    void* interfaceObjPtr;
    msgInterface_Interface_t currInterfaceObj;

    // Get the link of the next item on the session list.
    remSessionNextLinkPtr = GetNextDlsLink(&(sessionObjIterRef->sessionList),
                                        &(sessionObjIterRef->currSessionObj.link));

    while (remSessionNextLinkPtr == NULL)
    {
        interfaceObjPtr = GetNextInterfaceObjPtr((InterfaceObjIter_Ref_t)sessionObjIterRef);

        // There are no more interface objects. And therefore no more session objects.
        if (interfaceObjPtr == NULL)
        {
            return NULL;
        }

        // Read the interface object into our own memory.
        if (TargetReadAddress(PidToInspect, (uintptr_t)interfaceObjPtr,
                              &(currInterfaceObj), sizeof(currInterfaceObj)) != LE_OK)
        {
            INTERNAL_ERR(REMOTE_READ_ERR("interface object"));
        }

        // Update the session list in the iterator. Also reset the list head.
        sessionObjIterRef->sessionList.List = currInterfaceObj.sessionList;
        sessionObjIterRef->sessionList.headLinkPtr = NULL;

        // Get the link of the next item on the session list.
        remSessionNextLinkPtr = GetNextDlsLink(&(sessionObjIterRef->sessionList), NULL);
    }

    // Get the remote address of the session object.
    msgSession_UnixSession_t* remSessionObjPtr = CONTAINER_OF(remSessionNextLinkPtr,
                                                              msgSession_UnixSession_t, link);

    // Read the session object into our own memory.
    if (TargetReadAddress(PidToInspect, (uintptr_t)remSessionObjPtr,
                          &(sessionObjIterRef->currSessionObj),
                          sizeof(sessionObjIterRef->currSessionObj)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("session object"));
    }

    return &(sessionObjIterRef->currSessionObj);
}


// TODO: migrate the above to a separate module.
//--------------------------------------------------------------------------------------------------
/**
 * Prints help to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    puts(
        "NAME:\n"
        "    inspect - Inspects the internal structures such as memory pools, timers, etc. of a\n"
        "              Legato process.\n"
        "\n"
        "SYNOPSIS:\n"
        "    inspect <pools|threads|timers|mutexes|semaphores> [OPTIONS] PID\n"
        "    inspect ipc <servers|clients [sessions]> [OPTIONS] PID\n"
        "\n"
        "DESCRIPTION:\n"
        "    inspect pools              Prints the memory pools usage for the specified process.\n"
        "    inspect threads            Prints the info of threads for the specified process.\n"
        "    inspect timers             Prints the info of timers in all threads for the"
                                        " specified process.\n"
        "    inspect mutexes            Prints the info of mutexes in all threads for the"
                                        " specified process.\n"
        "    inspect semaphores         Prints the info of semaphores in all threads for the"
                                        " specified process.\n"
        "    inspect ipc                Prints the info of ipc in all threads for the"
                                        " specified process.\n"
        "\n"
        "OPTIONS:\n"
        "    -f\n"
        "        Periodically prints updated information for the process.\n"
        "\n"
        "    -v\n"
        "        Prints in verbose mode.\n"
        "\n"
        "    --interval=SECONDS\n"
        "        Prints updated information every SECONDS.\n"
        "\n"
        "    --format=json\n"
        "        Outputs the inspection results in JSON format.\n"
        "\n"
        "    --help\n"
        "        Display this help and exit.\n"
        );

    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Object describing a column of a display table. Multiple columns make up a display table.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char* colTitle;      ///< Column title.
    char* titleFormat;   ///< Format template for the column title.
    char* colField;      ///< Column field.
    char* fieldFormat;   ///< Format template for a column field.
    uint8_t maxDataSize; ///< Max data size. For strings, string length; otherwise, data size in
                         ///< number of bytes.
    // TODO: can probably be figured out from format template
    bool isString;       ///< Is the field string or not.
    uint8_t colWidth;    ///< Column width in number of characters.
    bool isPrintSimple;  ///< Print this field in non-verbose mode or not.
}
ColumnInfo_t;


//--------------------------------------------------------------------------------------------------
/**
 * Characters representing dividers between columns.
 */
//--------------------------------------------------------------------------------------------------
static char ColumnSpacers[] = " | ";


//--------------------------------------------------------------------------------------------------
/**
 * Line buffer and its associated char length and byte size.
 */
//--------------------------------------------------------------------------------------------------
static size_t TableLineLen = 0;
#define TableLineBytes (TableLineLen + 1)
static char* TableLineBuffer;


//--------------------------------------------------------------------------------------------------
/**
 * Strings representing sub-pool and super-pool.
 */
//--------------------------------------------------------------------------------------------------
static char SubPoolStr[] = "(Sub-pool)";
static char SuperPoolStr[] = "";


//--------------------------------------------------------------------------------------------------
/**
 * These tables define the display tables of each inspection type. The column width is left at 0
 * here, but will be calculated in InitDisplayTable. The calculated column width is guaranteed to
 * accomodate all possible data, so long as maxDataSize and isString are correctly populated. The
 * 0 maxDataSize fields are populated in InitDisplay. A column title acts as an identifier so they
 * need to be unique. The order of the ColumnInfo_t structs directly affect the order they are
 * displayed at runtime (column with the smallest index is at the leftmost side).
 */
//--------------------------------------------------------------------------------------------------
static ColumnInfo_t MemPoolTableInfo[] =
{
    {"TOTAL BLKS",  "%*s",  NULL, "%*zu",       sizeof(size_t),              false, 0, true},
    {"USED BLKS",   "%*s",  NULL, "%*zu",       sizeof(size_t),              false, 0, true},
    {"MAX USED",    "%*s",  NULL, "%*zu",       sizeof(size_t),              false, 0, true},
    {"OVERFLOWS",   "%*s",  NULL, "%*zu",       sizeof(size_t),              false, 0, true},
    {"ALLOCS",      "%*s",  NULL, "%*"PRIu64"", sizeof(uint64_t),            false, 0, true},
    {"BLK BYTES",   "%*s",  NULL, "%*zu",       sizeof(size_t),              false, 0, true},
    {"USED BYTES",  "%*s",  NULL, "%*zu",       sizeof(size_t),              false, 0, true},
    {"MEMORY POOL", "%-*s", NULL, "%-*s",       LIMIT_MAX_MEM_POOL_NAME_LEN, true,  0, true},
    {"SUB-POOL",    "%*s",  NULL, "%*s",        0,                           true,  0, true}
};
static size_t MemPoolTableInfoSize = NUM_ARRAY_MEMBERS(MemPoolTableInfo);

static ColumnInfo_t ThreadObjTableInfo[] =
{
    {"NAME",             "%*s", NULL, "%*s",  MAX_THREAD_NAME_SIZE, true,  0, true},
    {"JOINABLE",         "%*s", NULL, "%*u",  sizeof(bool),         false, 0, true},
    {"STARTED",          "%*s", NULL, "%*u",  sizeof(bool),         false, 0, true},
    {"DETACHSTATE",      "%*s", NULL, "%*s",  0,                    true,  0, true},
    {"SCHED POLICY",     "%*s", NULL, "%*s",  0,                    true,  0, true},
    {"SCHED PARAM",      "%*s", NULL, "%*u",  sizeof(int),          false, 0, true},
    {"INHERIT SCHED",    "%*s", NULL, "%*s",  0,                    true,  0, true},
    {"CONTENTION SCOPE", "%*s", NULL, "%*s",  0,                    true,  0, true},
    {"GUARD SIZE",       "%*s", NULL, "%*zu", sizeof(size_t),       false, 0, true},
    {"STACK ADDR",       "%*s", NULL, "%*X",  sizeof(uint64_t),     false, 0, true},
    {"STACK SIZE",       "%*s", NULL, "%*zu", sizeof(size_t),       false, 0, true}
};
static size_t ThreadObjTableInfoSize = NUM_ARRAY_MEMBERS(ThreadObjTableInfo);

static ColumnInfo_t TimerTableInfo[] =
{
    {"NAME",         "%*s", NULL, "%*s",  LIMIT_MAX_TIMER_NAME_BYTES, true,  0, true},
    {"INTERVAL",     "%*s", NULL, "%*f",  sizeof(double),             false, 0, true},
    {"REPEAT COUNT", "%*s", NULL, "%*u",  sizeof(uint32_t),           false, 0, true},
    {"ISACTIVE",     "%*s", NULL, "%*u",  sizeof(bool),               false, 0, true},
    {"EXPIRY TIME",  "%*s", NULL, "%*f",  sizeof(double),             false, 0, true},
    {"EXPIRY COUNT", "%*s", NULL, "%*u",  sizeof(uint32_t),           false, 0, true}
};
static size_t TimerTableInfoSize = NUM_ARRAY_MEMBERS(TimerTableInfo);

static ColumnInfo_t MutexTableInfo[] =
{
    {"NAME",         "%*s", NULL, "%*s", MAX_NAME_BYTES,       true,  0, true},
    {"LOCK COUNT",   "%*s", NULL, "%*d", sizeof(int),          false, 0, true},
    {"RECURSIVE",    "%*s", NULL, "%*u", sizeof(bool),         false, 0, true},
    {"WAITING LIST", "%*s", NULL, "%*s", MAX_THREAD_NAME_SIZE, true,  0, true}
};
static size_t MutexTableInfoSize = NUM_ARRAY_MEMBERS(MutexTableInfo);

static ColumnInfo_t SemaphoreTableInfo[] =
{
    {"NAME",         "%*s", NULL, "%*s", LIMIT_MAX_SEMAPHORE_NAME_BYTES, true,  0, true},
    {"WAITING LIST", "%*s", NULL, "%*s", MAX_THREAD_NAME_SIZE,           true,  0, true}
};
static size_t SemaphoreTableInfoSize = NUM_ARRAY_MEMBERS(SemaphoreTableInfo);

static ColumnInfo_t ServiceObjTableInfo[] =
{
    {"INTERFACE NAME", "%*s", NULL, "%*s",  LIMIT_MAX_IPC_INTERFACE_NAME_BYTES, true,  0, true},
    {"STATE",          "%*s", NULL, "%*s",  0,                                  true,  0, true},
    {"THREAD NAME",    "%*s", NULL, "%*s",  MAX_THREAD_NAME_SIZE,               true,  0, true},
    {"PROTOCOL ID",    "%*s", NULL, "%*s",  LIMIT_MAX_PROTOCOL_ID_BYTES,        true,  0, false},
    {"MAX PAYLOAD",    "%*s", NULL, "%*zu", sizeof(size_t),                     false, 0, false},
    {"FD",             "%*s", NULL, "%*d",  sizeof(int),                        false, 0, false}
};
static size_t ServiceObjTableInfoSize = NUM_ARRAY_MEMBERS(ServiceObjTableInfo);

static ColumnInfo_t ClientObjTableInfo[] =
{
    {"INTERFACE NAME", "%*s", NULL, "%*s",  LIMIT_MAX_IPC_INTERFACE_NAME_BYTES, true,  0, true},
    {"PROTOCOL ID",    "%*s", NULL, "%*s",  LIMIT_MAX_PROTOCOL_ID_BYTES,        true,  0, false},
    {"MAX PAYLOAD",    "%*s", NULL, "%*zu", sizeof(size_t),                     false, 0, false},
};
static size_t ClientObjTableInfoSize = NUM_ARRAY_MEMBERS(ClientObjTableInfo);

static ColumnInfo_t SessionObjTableInfo[] =
{
    {"INTERFACE NAME", "%*s", NULL, "%*s", LIMIT_MAX_IPC_INTERFACE_NAME_BYTES, true,  0, true},
    {"STATE",          "%*s", NULL, "%*s", 0,                                  true,  0, true},
    {"THREAD NAME",    "%*s", NULL, "%*s", MAX_THREAD_NAME_SIZE,               true,  0, true},
    {"FD",             "%*s", NULL, "%*d", sizeof(int),                        false, 0, false}
};
static size_t SessionObjTableInfoSize = NUM_ARRAY_MEMBERS(SessionObjTableInfo);


//--------------------------------------------------------------------------------------------------
/**
 * These tables define the mapping between enum/define and their textual representation.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t defn; ///< Actual number for the enum or define.
    char str[50];  ///< Textual representation for the enum/define.
}
DefnStrMapping_t;

// pthread attribute: detach state
static DefnStrMapping_t ThreadObjDetachStateTbl[] =
{
    {
        PTHREAD_CREATE_DETACHED,
        "PTHREAD_CREATE_DETACHED"
    },
    {
        PTHREAD_CREATE_JOINABLE,
        "PTHREAD_CREATE_JOINABLE"
    }
};
static int ThreadObjDetachStateTblSize = NUM_ARRAY_MEMBERS(ThreadObjDetachStateTbl);

// pthread attribute: shceduling policy
static DefnStrMapping_t ThreadObjSchedPolTbl[] =
{
    {
        SCHED_FIFO,
        "SCHED_FIFO"
    },
    {
        SCHED_RR,
        "SCHED_RR"
    },

    {
        SCHED_OTHER,
        "SCHED_OTHER"
    }
};
static int ThreadObjSchedPolTblSize = NUM_ARRAY_MEMBERS(ThreadObjSchedPolTbl);

// pthread attribute: inherite scheduler
static DefnStrMapping_t ThreadObjInheritSchedTbl[] =
{
    {
        PTHREAD_INHERIT_SCHED,
        "PTHREAD_INHERIT_SCHED"
    },
    {
        PTHREAD_EXPLICIT_SCHED,
        "PTHREAD_EXPLICIT_SCHED"
    }
};
static int ThreadObjInheritSchedTblSize = NUM_ARRAY_MEMBERS(ThreadObjInheritSchedTbl);

// pthread attribute: contention scope
static DefnStrMapping_t ThreadObjContentionScopeTbl[] =
{
    {
        PTHREAD_SCOPE_SYSTEM,
        "PTHREAD_SCOPE_SYSTEM"
    },
    {
        PTHREAD_SCOPE_PROCESS,
        "PTHREAD_SCOPE_PROCESS"
    }
};
static int ThreadObjContentionScopeTblSize = NUM_ARRAY_MEMBERS(ThreadObjContentionScopeTbl);

// service state
static DefnStrMapping_t ServiceStateTbl[] =
{
    {
        LE_MSG_INTERFACE_SERVICE_CONNECTING,
        "connecting"
    },
    {
        LE_MSG_INTERFACE_SERVICE_ADVERTISED,
        "advertised"
    },
    {
        LE_MSG_INTERFACE_SERVICE_HIDDEN,
        "hidden"
    }
};
static int ServiceStateTblSize = NUM_ARRAY_MEMBERS(ServiceStateTbl);

// session state
static DefnStrMapping_t SessionStateTbl[] =
{
    {
        LE_MSG_SESSION_STATE_CLOSED,
        "closed"
    },
    {
        LE_MSG_SESSION_STATE_OPENING,
        "waiting"
    },
    {
        LE_MSG_SESSION_STATE_OPEN,
        "open"
    }
};
static int SessionStateTblSize = NUM_ARRAY_MEMBERS(SessionStateTbl);


//--------------------------------------------------------------------------------------------------
/**
 * Looks up the mapping between an enum or define and its textual description.
 *
 * @return
 *      A pointer to the textual description in the table.
 */
//--------------------------------------------------------------------------------------------------
static char* DefnToStr
(
    int defn,                ///< [IN] The enum/define that we want to look up the description for.
    DefnStrMapping_t* table, ///< [IN] Pointer to the table to look up the textual description.
    int tableSize            ///< [IN] Table size.
)
{
    int i;
    for (i = 0; i < tableSize; i++)
    {
        if (defn == table[i].defn)
        {
            return table[i].str;
        }
    }

    // In case the defn is invalid, terminate the program.
    INTERNAL_ERR("Invalid define - failed to look up its textual representation.");
}


//--------------------------------------------------------------------------------------------------
/**
 * For a given table of number and text, find out the max number of characters out of all text.
 */
//--------------------------------------------------------------------------------------------------
static size_t FindMaxStrSizeFromTable
(
    DefnStrMapping_t* table, ///< [IN] Table ref.
    size_t tableSize         ///< [IN] Table size.
)
{
    size_t cur, max = 0;
    int i;
    for (i = 0; i < tableSize; i++)
    {
        cur = strlen(table[i].str);
        if (cur > max)
        {
            max = cur;
        }
    }
    return max;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the max data size of the specified column in the specified table.
 */
//--------------------------------------------------------------------------------------------------
static void InitDisplayTableMaxDataSize
(
    char* colTitle,      ///< [IN] As a key for lookup, title of column to print the string under.
    ColumnInfo_t* table, ///< [IN] Table ref.
    size_t tableSize,    ///< [IN] Table size.
    size_t maxDataSize   ///< [IN] Max data size to init the column with.
)
{
    int i;
    for (i = 0; i < tableSize; i++)
    {
        if (strcmp(table[i].colTitle, colTitle) == 0)
        {
            table[i].maxDataSize = maxDataSize;
            return;
        }
    }

    INTERNAL_ERR("Failed to init display table.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize a display table. This function calculates the appropriate column widths that will
 * accomodate all possible data for each column. With column widths calculated, column and line
 * buffers are also properly initialized.
 */
//--------------------------------------------------------------------------------------------------
static void InitDisplayTable
(
    ColumnInfo_t* table, ///< [IN] Table ref.
    size_t tableSize     ///< [IN] Table size.
)
{
    // Some columns in ThreadObjTableInfo needs its maxDataSize figured out.
    // Determine largest text size out of all possible text in a number-text table; update the
    // sizes to the thread object display table.
    if (table == ThreadObjTableInfo)
    {
        InitDisplayTableMaxDataSize("DETACHSTATE", table, tableSize,
                                    FindMaxStrSizeFromTable(ThreadObjDetachStateTbl,
                                                            ThreadObjDetachStateTblSize));

        InitDisplayTableMaxDataSize("SCHED POLICY", table, tableSize,
                                    FindMaxStrSizeFromTable(ThreadObjSchedPolTbl,
                                                            ThreadObjSchedPolTblSize));

        InitDisplayTableMaxDataSize("INHERIT SCHED", table, tableSize,
                                    FindMaxStrSizeFromTable(ThreadObjInheritSchedTbl,
                                                            ThreadObjInheritSchedTblSize));

        InitDisplayTableMaxDataSize("CONTENTION SCOPE", table, tableSize,
                                    FindMaxStrSizeFromTable(ThreadObjContentionScopeTbl,
                                                            ThreadObjContentionScopeTblSize));
    }
    else if (table == MemPoolTableInfo)
    {
        size_t subPoolStrLen = strlen(SubPoolStr);
        size_t superPoolStrLen = strlen(SuperPoolStr);
        size_t subPoolColumnStrLen = subPoolStrLen > superPoolStrLen ? subPoolStrLen  :
                                                                       superPoolStrLen;
        InitDisplayTableMaxDataSize("SUB-POOL", table, tableSize, subPoolColumnStrLen);
    }
    else if (table == ServiceObjTableInfo)
    {
        InitDisplayTableMaxDataSize("STATE", table, tableSize,
                                    FindMaxStrSizeFromTable(ServiceStateTbl,
                                                            ServiceStateTblSize));
    }
    else if (table == SessionObjTableInfo)
    {
        InitDisplayTableMaxDataSize("STATE", table, tableSize,
                                    FindMaxStrSizeFromTable(SessionStateTbl,
                                                            SessionStateTblSize));
    }

    int i;
    for (i = 0; i < tableSize; i++)
    {
        size_t headerTextWidth = strlen(table[i].colTitle);

        if (table[i].isString == false)
        {
            // uint8_t should be plenty enough to store the number of digits of even uint64_t
            // (which is 20). casting the result of log10 could overflow but highly unlikely.
            uint8_t maxDataWidth = (uint8_t)log10(exp2(table[i].maxDataSize*8))+1;
            table[i].colWidth = (maxDataWidth > headerTextWidth) ? maxDataWidth : headerTextWidth;
        }
        else
        {
            table[i].colWidth = (table[i].maxDataSize > headerTextWidth) ?
                                 table[i].maxDataSize : headerTextWidth;
        }

        // Now that column width is figured out, we can use that to allocate buffer for colField.
        #define colBytes table[i].colWidth + 1
        table[i].colField = (char*)malloc(colBytes);
        // Initialize the buffer
        memset(table[i].colField, 0, colBytes);
        #undef colBytes

        // Add the column width and column spacer length to the overall line length.
        TableLineLen += table[i].colWidth + strlen(ColumnSpacers);
    }

    // allocate and init the line buffer
    TableLineBuffer = (char*)malloc(TableLineBytes);
    if (!TableLineBuffer)
    {
       INTERNAL_ERR("TableLineBuffer is NULL.");
       return;
    }
    memset(TableLineBuffer, 0, TableLineBytes);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize all display tables, and other misc. display related chores.
 */
//--------------------------------------------------------------------------------------------------
static void InitDisplay
(
    InspType_t inspectType ///< [IN] What to inspect.
)
{
    switch (inspectType)
    {
        case INSPECT_INSP_TYPE_MEM_POOL:
            // Initialize the display tables with the optimal column widths.
            InitDisplayTable(MemPoolTableInfo, MemPoolTableInfoSize);
            break;

        case INSPECT_INSP_TYPE_THREAD_OBJ:
            InitDisplayTable(ThreadObjTableInfo, ThreadObjTableInfoSize);
            break;

        case INSPECT_INSP_TYPE_TIMER:
            InitDisplayTable(TimerTableInfo, TimerTableInfoSize);
            break;

        case INSPECT_INSP_TYPE_MUTEX:
            InitDisplayTable(MutexTableInfo, MutexTableInfoSize);
            break;

        case INSPECT_INSP_TYPE_SEMAPHORE:
            InitDisplayTable(SemaphoreTableInfo, SemaphoreTableInfoSize);
            break;

        case INSPECT_INSP_TYPE_IPC_SERVERS:
            InitDisplayTable(ServiceObjTableInfo, ServiceObjTableInfoSize);
            break;

        case INSPECT_INSP_TYPE_IPC_CLIENTS:
            InitDisplayTable(ClientObjTableInfo, ClientObjTableInfoSize);
            break;

        case INSPECT_INSP_TYPE_IPC_SERVERS_SESSIONS:
        case INSPECT_INSP_TYPE_IPC_CLIENTS_SESSIONS:
            InitDisplayTable(SessionObjTableInfo, SessionObjTableInfoSize);
            break;

        default:
            INTERNAL_ERR("Failed to initialize display table - unexpected inspect type %d.",
                         inspectType);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints the header row from the specified table.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHeader
(
    ColumnInfo_t* table, ///< [IN] Table ref.
    size_t tableSize     ///< [IN] Table size.
)
{
    int index = 0;
    int i = 0;
    while (i < tableSize)
    {
        if ((table[i].isPrintSimple == true) || (IsVerbose == true))
        {
            index += snprintf((TableLineBuffer + index), (TableLineBytes - index),
                              table[i].titleFormat, table[i].colWidth, table[i].colTitle);
            index += snprintf((TableLineBuffer + index), (TableLineBytes - index), "%s",
                              ColumnSpacers);
        }

        i++;
    }
    puts(TableLineBuffer);
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints a row for the currently inspected node from the specified table. The column buffers
 * (colField) need to be filled in prior to calling this function.
 */
//--------------------------------------------------------------------------------------------------
static void PrintInfo
(
    ColumnInfo_t* table, ///< [IN] Table ref.
    size_t tableSize     ///< [IN] Table size.
)
{
    int index = 0;
    int i = 0;
    while (i < tableSize)
    {
        if ((table[i].isPrintSimple == true) || (IsVerbose == true))
        {
            index += snprintf((TableLineBuffer + index), (TableLineBytes - index), "%s",
                              table[i].colField);
            index += snprintf((TableLineBuffer + index), (TableLineBytes - index), "%s",
                              ColumnSpacers);
        }

        i++;
    }
    puts(TableLineBuffer);
}


//--------------------------------------------------------------------------------------------------
/**
 * If information spans across multiple lines, or if something needs to be printed under only a
 * certain column, this function prints the specified string under the specified table and column.
 * Nothing is printed for all other columns and no column spacers are printed.
 */
//--------------------------------------------------------------------------------------------------
static void PrintUnderColumn
(
    char* colTitle,      ///< [IN] As a key for lookup, title of column to print the string under.
    ColumnInfo_t* table, ///< [IN] Table ref.
    size_t tableSize,    ///< [IN] Table size.
    char* str            ///< [IN] String to print under the specified column.
)
{
    int index = 0;
    int i = 0;
    while (i < tableSize)
    {
        if (strcmp(table[i].colTitle, colTitle) == 0)
        {
            index += snprintf((TableLineBuffer + index), (TableLineBytes - index), "%*s",
                              table[i].colWidth, str);
        }
        else
        {
            index += snprintf((TableLineBuffer + index), (TableLineBytes - index), "%*s",
                              table[i].colWidth, "");
        }

        index += snprintf((TableLineBuffer + index), (TableLineBytes - index), "%*s",
                          (int)strlen(ColumnSpacers), "");
        i++;
    }
    puts(TableLineBuffer);
}


//--------------------------------------------------------------------------------------------------
/**
 * For the given table, return the next column.
 */
//--------------------------------------------------------------------------------------------------
static ColumnInfo_t* GetNextColumn
(
    ColumnInfo_t* table, ///< [IN] Table ref.
    size_t tableSize,    ///< [IN] Table size.
    int* indexRef        ///< [IN/OUT] Iterator to parse the table.
)
{
    int i = *indexRef;

    if (i == tableSize)
    {
        INTERNAL_ERR("Unable to get the next column.");
    }

    (*indexRef)++;

    return &(table[i]);
}


//--------------------------------------------------------------------------------------------------
/**
 * Print Inspect results header for human-readable format; and print global data for machine-
 * readable format.
 *
 * @return
 *      The number of lines printed, if outputting human-readable format.
 */
//--------------------------------------------------------------------------------------------------
static int PrintInspectHeader
(
    void
)
{
    int lineCount = 0;
    ColumnInfo_t* table;
    size_t tableSize;

    // The size should accomodate the longest inspectTypeString.
    #define inspectTypeStringSize 40
    char inspectTypeString[inspectTypeStringSize];
    switch (InspectType)
    {
        case INSPECT_INSP_TYPE_MEM_POOL:
            strncpy(inspectTypeString, "Memory Pools", inspectTypeStringSize);
            table = MemPoolTableInfo;
            tableSize = MemPoolTableInfoSize;
            break;

        case INSPECT_INSP_TYPE_THREAD_OBJ:
            strncpy(inspectTypeString, "Thread Objects", inspectTypeStringSize);
            table = ThreadObjTableInfo;
            tableSize = ThreadObjTableInfoSize;
            break;

        case INSPECT_INSP_TYPE_TIMER:
            strncpy(inspectTypeString, "Timers", inspectTypeStringSize);
            table = TimerTableInfo;
            tableSize = TimerTableInfoSize;
            break;

        case INSPECT_INSP_TYPE_MUTEX:
            strncpy(inspectTypeString, "Mutexes", inspectTypeStringSize);
            table = MutexTableInfo;
            tableSize = MutexTableInfoSize;
            break;

        case INSPECT_INSP_TYPE_SEMAPHORE:
            strncpy(inspectTypeString, "Semaphores", inspectTypeStringSize);
            table = SemaphoreTableInfo;
            tableSize = SemaphoreTableInfoSize;
            break;

        case INSPECT_INSP_TYPE_IPC_SERVERS:
            strncpy(inspectTypeString, "IPC Server Interface", inspectTypeStringSize);
            table = ServiceObjTableInfo;
            tableSize = ServiceObjTableInfoSize;
            break;

        case INSPECT_INSP_TYPE_IPC_CLIENTS:
            strncpy(inspectTypeString, "IPC Client Interface", inspectTypeStringSize);
            table = ClientObjTableInfo;
            tableSize = ClientObjTableInfoSize;
            break;

        case INSPECT_INSP_TYPE_IPC_SERVERS_SESSIONS:
            strncpy(inspectTypeString, "IPC Server Interface Sessions", inspectTypeStringSize);
            table = SessionObjTableInfo;
            tableSize = SessionObjTableInfoSize;
            break;

        case INSPECT_INSP_TYPE_IPC_CLIENTS_SESSIONS:
            strncpy(inspectTypeString, "IPC Client Interface Sessions", inspectTypeStringSize);
            table = SessionObjTableInfo;
            tableSize = SessionObjTableInfoSize;
            break;

        default:
            INTERNAL_ERR("unexpected inspect type %d.", InspectType);
    }

    if (!IsOutputJson)
    {
        printf("\n");
        lineCount++;

        // Print title.
        printf("Legato %s Inspector\n", inspectTypeString);
        lineCount++;
        printf("Inspecting process %d\n", PidToInspect);
        lineCount++;

        // Print column headers.
        PrintHeader(table, tableSize);
        lineCount++;
    }
    else
    {
        // The beginning curly brace of the "main" JSON object, and the beginning of the "Headers"
        // data.
        printf("{\"Headers\":[");

        // Print the column headers.
        int i;
        bool printed = false;
        for (i = 0; i < tableSize; i++)
        {
            if ((table[i].isPrintSimple == true) || (IsVerbose == true))
            {
                if (printed == true)
                {
                    printf(",");
                }
                else
                {
                    printed = true;
                }

                printf("\"%s\"", table[i].colTitle);
            }
        }

        printf("],");

        // Print the data of "InspectType", "PID", and the beginning of "Data".
        printf("\"InspectType\":\"%s\",\"PID\":\"%d\",\"Data\":[", inspectTypeString, PidToInspect);
    }

    return lineCount;
}



//--------------------------------------------------------------------------------------------------
/**
 * The ExportXXXToJson and FillXXXColField families of functions are used by the PrintXXXInfo
 * functions to output data in json format, or to print the "colField" string in the XXXTableInfo
 * tables (which are to be later printed to the terminal), respectively.
 *
 * These functions provide type checking for the data to be printed, and properly format the data
 * according to the formatting rules defined by the XXXTableInfo tables. They also determine of the
 * data should be output or not based on the "verbose mode" setting.
 *
 * For each data type used by the XXXTableInfo tables, there should be a corresponding pairs of
 * ExportXXXToJson and FillXXXColField functions.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The ExportXXXToJson family of functions.
 */
//--------------------------------------------------------------------------------------------------

// The "array" can contain any valid json values, represented by strings.
// Before calling this function, the formatting should have been taken cared of for the data in the
// array.
static void ExportArrayToJson
(
    char*         array,     ///< [IN] json array.
    ColumnInfo_t* table,     ///< [IN] XXXTableInfo ref.
    size_t        tableSize, ///< [IN] XXXTableInfo size.
    int*          indexRef,  ///< [IN/OUT] iterator to parse the table.
    bool*         printed    ///< [IN/OUT] if the first entry is printed. if so, print a leading
                             ///<          comma for the current entry.
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        if (*printed == true)
        {
            printf(",");
        }
        else
        {
            *printed = true;
        }

        printf("%s", array);
    }
}

// string
// double quotes are added per json standard.
static void ExportStrToJson
(
    char*         field,     ///< [IN] the data to be exported to json.
    ColumnInfo_t* table,     ///< [IN] XXXTableInfo ref.
    size_t        tableSize, ///< [IN] XXXTableInfo size.
    int*          indexRef,  ///< [IN/OUT] iterator to parse the table.
    bool*         printed    ///< [IN/OUT] if the first entry is printed. if so, print a leading
                             ///<          comma for the current entry.
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        if (*printed == true)
        {
            printf(",");
        }
        else
        {
            *printed = true;
        }

        printf("\"");
        printf(col->fieldFormat, 0, field);
        printf("\"");
    }
}

// Note that json has only a "number" type, so the export functions for all numbers such as size_t,
// int, uint32_t, etc. should have the same function content.
// size_t
static void ExportSizeTToJson
(
    size_t        field, // param comments same as ExportStrToJson
    ColumnInfo_t* table,
    size_t        tableSize,
    int*          indexRef,
    bool*         printed
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        if (*printed == true)
        {
            printf(",");
        }
        else
        {
            *printed = true;
        }

        printf(col->fieldFormat, 0, field);
    }
}

// int
static void ExportIntToJson
(
    int           field, // param comments same as ExportStrToJson
    ColumnInfo_t* table,
    size_t        tableSize,
    int*          indexRef,
    bool*         printed
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        if (*printed == true)
        {
            printf(",");
        }
        else
        {
            *printed = true;
        }

        printf(col->fieldFormat, 0, field);
    }
}

// double
static void ExportDoubleToJson
(
    double        field, // param comments same as ExportStrToJson
    ColumnInfo_t* table,
    size_t        tableSize,
    int*          indexRef,
    bool*         printed
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        if (*printed == true)
        {
            printf(",");
        }
        else
        {
            *printed = true;
        }

        printf(col->fieldFormat, 0, field);
    }
}

// uint32_t
static void ExportUint32ToJson
(
    uint32_t      field, // param comments same as ExportStrToJson
    ColumnInfo_t* table,
    size_t        tableSize,
    int*          indexRef,
    bool*         printed
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        if (*printed == true)
        {
            printf(",");
        }
        else
        {
            *printed = true;
        }

        printf(col->fieldFormat, 0, field);
    }
}

// uint64_t
static void ExportUint64ToJson
(
    uint64_t      field, // param comments same as ExportStrToJson
    ColumnInfo_t* table,
    size_t        tableSize,
    int*          indexRef,
    bool*         printed
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        if (*printed == true)
        {
            printf(",");
        }
        else
        {
            *printed = true;
        }

        printf(col->fieldFormat, 0, field);
    }
}

// bool
// "true" or "false" are outputted per json standard.
static void ExportBoolToJson
(
    bool          field, // param comments same as ExportStrToJson
    ColumnInfo_t* table,
    size_t        tableSize,
    int*          indexRef,
    bool*         printed
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        if (*printed == true)
        {
            printf(",");
        }
        else
        {
            *printed = true;
        }

        if (field == true)
        {
            printf("true");
        }
        else
        {
            printf("false");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * The FillXXXColField family of functions.
 */
//--------------------------------------------------------------------------------------------------

// string
static void FillStrColField
(
    char*         field,     ///< [IN] the data to be printed to the ColField of the table.
    ColumnInfo_t* table,     ///< [IN] XXXTableInfo ref.
    size_t        tableSize, ///< [IN] XXXTableInfo size.
    int*          indexRef   ///< [IN/OUT] iterator to parse the table.
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        snprintf(col->colField, (col->colWidth + 1), col->fieldFormat, col->colWidth, field);
    }
}

// size_t
static void FillSizeTColField
(
    size_t        field, // param comments same as FillStrColField
    ColumnInfo_t* table,
    size_t        tableSize,
    int*          indexRef
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        snprintf(col->colField, (col->colWidth + 1), col->fieldFormat, col->colWidth, field);
    }
}

// int
static void FillIntColField
(
    int           field, // param comments same as FillStrColField
    ColumnInfo_t* table,
    size_t        tableSize,
    int*          indexRef
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        snprintf(col->colField, (col->colWidth + 1), col->fieldFormat, col->colWidth, field);
    }
}

// double
static void FillDoubleColField
(
    double        field, // param comments same as FillStrColField
    ColumnInfo_t* table,
    size_t        tableSize,
    int*          indexRef
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        snprintf(col->colField, (col->colWidth + 1), col->fieldFormat, col->colWidth, field);
    }
}

// uint32_t
static void FillUint32ColField
(
    uint32_t      field, // param comments same as FillStrColField
    ColumnInfo_t* table,
    size_t        tableSize,
    int*          indexRef
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        snprintf(col->colField, (col->colWidth + 1), col->fieldFormat, col->colWidth, field);
    }
}

// uint64_t
static void FillUint64ColField
(
    uint64_t      field, // param comments same as FillStrColField
    ColumnInfo_t* table,
    size_t        tableSize,
    int*          indexRef
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        snprintf(col->colField, (col->colWidth + 1), col->fieldFormat, col->colWidth, field);
    }
}

// bool
// "T" or "F" are printed instead of "1" or "0".
static void FillBoolColField
(
    bool          field, // param comments same as FillStrColField
    ColumnInfo_t* table,
    size_t        tableSize,
    int*          indexRef
)
{
    ColumnInfo_t* col = GetNextColumn(table, tableSize, indexRef);
    if ((col->isPrintSimple == true) || (IsVerbose == true))
    {
        if (field == true)
        {
            snprintf(col->colField, (col->colWidth + 1), "%*s", col->colWidth, "T");
        }
        else
        {
            snprintf(col->colField, (col->colWidth + 1), "%*s", col->colWidth, "F");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * For outputting JSON format. If the node printed is not the first one, print a comma first to
 * delimit from the last printed node.
 */
//--------------------------------------------------------------------------------------------------
static bool IsPrintedNodeFirst = true;


//--------------------------------------------------------------------------------------------------
/**
 * Print memory pool information to stdout.
 */
//--------------------------------------------------------------------------------------------------
static int PrintMemPoolInfo
(
    le_mem_PoolRef_t memPool    ///< [IN] ref to mem pool to be printed.
)
{
    int lineCount = 0;

    // Get pool stats.
    le_mem_PoolStats_t poolStats;
    le_mem_GetStats(memPool, &poolStats);

    size_t blockSize = le_mem_GetObjectFullSize(memPool);

    // Determine if this pool is a sub-pool, and set the appropriate string to display it.
    char* subPoolStr = le_mem_IsSubPool(memPool) ? SubPoolStr : SuperPoolStr;

    // Get the pool name.
    char name[LIMIT_MAX_COMPONENT_NAME_LEN + 1 + LIMIT_MAX_MEM_POOL_NAME_BYTES];
    INTERNAL_ERR_IF(le_mem_GetName(memPool, name, sizeof(name)) != LE_OK,
                    "Name buffer is too small.");

    // Output mem pool info
    int index = 0;

    if (!IsOutputJson)
    {
        // NOTE that the order has to correspond to the column orders in the corresponding table.
        // Since this order is "hardcoded" in a sense, one should avoid having multiple
        // copies of these. The same applies to other PrintXXXInfo functions.
        FillSizeTColField (le_mem_GetObjectCount(memPool),       MemPoolTableInfo,
                                                                 MemPoolTableInfoSize, &index);
        FillSizeTColField (poolStats.numBlocksInUse,             MemPoolTableInfo,
                                                                 MemPoolTableInfoSize, &index);
        FillSizeTColField (poolStats.maxNumBlocksUsed,           MemPoolTableInfo,
                                                                 MemPoolTableInfoSize, &index);
        FillSizeTColField (poolStats.numOverflows,               MemPoolTableInfo,
                                                                 MemPoolTableInfoSize, &index);
        FillUint64ColField(poolStats.numAllocs,                  MemPoolTableInfo,
                                                                 MemPoolTableInfoSize, &index);
        FillSizeTColField (blockSize,                            MemPoolTableInfo,
                                                                 MemPoolTableInfoSize, &index);
        FillSizeTColField (blockSize*(poolStats.numBlocksInUse), MemPoolTableInfo,
                                                                 MemPoolTableInfoSize, &index);
        FillStrColField   (name,                                 MemPoolTableInfo,
                                                                 MemPoolTableInfoSize, &index);
        FillStrColField   (subPoolStr,                           MemPoolTableInfo,
                                                                 MemPoolTableInfoSize, &index);

        PrintInfo(MemPoolTableInfo, MemPoolTableInfoSize);
        lineCount++;
    }
    else
    {
        // If it's not the first time, print a comma.
        if (!IsPrintedNodeFirst)
        {
            printf(",");
        }
        else
        {
            IsPrintedNodeFirst = false;
        }

        bool printed = false;

        printf("[");

        ExportSizeTToJson (le_mem_GetObjectCount(memPool),  MemPoolTableInfo,
                                                            MemPoolTableInfoSize, &index, &printed);
        ExportSizeTToJson (poolStats.numBlocksInUse,        MemPoolTableInfo,
                                                            MemPoolTableInfoSize, &index, &printed);
        ExportSizeTToJson (poolStats.maxNumBlocksUsed,      MemPoolTableInfo,
                                                            MemPoolTableInfoSize, &index, &printed);
        ExportSizeTToJson (poolStats.numOverflows,          MemPoolTableInfo,
                                                            MemPoolTableInfoSize, &index, &printed);
        ExportUint64ToJson(poolStats.numAllocs,             MemPoolTableInfo,
                                                            MemPoolTableInfoSize, &index, &printed);
        ExportSizeTToJson (blockSize,                       MemPoolTableInfo,
                                                            MemPoolTableInfoSize, &index, &printed);
        ExportSizeTToJson (blockSize*(poolStats.numBlocksInUse), MemPoolTableInfo,
                                                            MemPoolTableInfoSize, &index, &printed);
        ExportStrToJson   (name,                            MemPoolTableInfo,
                                                            MemPoolTableInfoSize, &index, &printed);
        ExportStrToJson   (subPoolStr,                      MemPoolTableInfo,
                                                            MemPoolTableInfoSize, &index, &printed);

        printf("]");
    }

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print thread obj information to stdout.
 */
//--------------------------------------------------------------------------------------------------
static int PrintThreadObjInfo
(
    thread_Obj_t* threadObjRef   ///< [IN] ref to thread obj to be printed.
)
{
    int lineCount = 0;

    int detachState;
    if (pthread_attr_getdetachstate(&threadObjRef->attr, &detachState) != 0)
    {
        INTERNAL_ERR("pthread_attr_getdetachstate failed.");
    }
    char* detachStateStr = DefnToStr(detachState, ThreadObjDetachStateTbl,
                                     ThreadObjDetachStateTblSize);

    int schedPolicy;
    if (pthread_attr_getschedpolicy(&threadObjRef->attr, &schedPolicy) != 0)
    {
        INTERNAL_ERR("pthread_attr_getschedpolicy failed.");
    }
    char* schedPolicyStr = DefnToStr(schedPolicy, ThreadObjSchedPolTbl, ThreadObjSchedPolTblSize);

    struct sched_param schedParam;
    if (pthread_attr_getschedparam(&threadObjRef->attr, &schedParam) != 0)
    {
        INTERNAL_ERR("pthread_attr_getschedparam failed.");
    }

    int inheritSched;
    if (pthread_attr_getinheritsched(&threadObjRef->attr, &inheritSched) != 0)
    {
        INTERNAL_ERR("pthread_attr_getinheritsched failed.");
    }
    char* inheritSchedStr = DefnToStr(inheritSched, ThreadObjInheritSchedTbl,
                                      ThreadObjInheritSchedTblSize);

    int contentionScope;
    if (pthread_attr_getscope(&threadObjRef->attr, &contentionScope) != 0)
    {
        INTERNAL_ERR("pthread_attr_getscope failed.");
    }
    char* contentionScopeStr = DefnToStr(contentionScope, ThreadObjContentionScopeTbl,
                                         ThreadObjContentionScopeTblSize);

    size_t guardSize;
    if (pthread_attr_getguardsize(&threadObjRef->attr, &guardSize) != 0)
    {
        INTERNAL_ERR("pthread_attr_getguardsize failed.");
    }

    uint32_t stackAddr[1]; // Need to handle both 32 and 64-bit platforms
    size_t stackSize;
    if (pthread_attr_getstack(&threadObjRef->attr, (void**)&stackAddr, &stackSize) != 0)
    {
        INTERNAL_ERR("pthread_attr_getstack failed.");
    }

    // Output thread object info
    int index = 0;

    if (!IsOutputJson)
    {
        FillStrColField   (THREAD_NAME(threadObjRef->name),         ThreadObjTableInfo,
                                                                    ThreadObjTableInfoSize, &index);
        FillBoolColField  (threadObjRef->isJoinable,                ThreadObjTableInfo,
                                                                    ThreadObjTableInfoSize, &index);
        FillBoolColField  ((threadObjRef->state != THREAD_STATE_NEW), ThreadObjTableInfo,
                                                                    ThreadObjTableInfoSize, &index);
        FillStrColField   (detachStateStr,                          ThreadObjTableInfo,
                                                                    ThreadObjTableInfoSize, &index);
        FillStrColField   (schedPolicyStr,                          ThreadObjTableInfo,
                                                                    ThreadObjTableInfoSize, &index);
        FillIntColField   (schedParam.sched_priority,               ThreadObjTableInfo,
                                                                    ThreadObjTableInfoSize, &index);
        FillStrColField   (inheritSchedStr,                         ThreadObjTableInfo,
                                                                    ThreadObjTableInfoSize, &index);
        FillStrColField   (contentionScopeStr,                      ThreadObjTableInfo,
                                                                    ThreadObjTableInfoSize, &index);
        FillSizeTColField (guardSize,                               ThreadObjTableInfo,
                                                                    ThreadObjTableInfoSize, &index);
        FillUint64ColField(stackAddr[0],                            ThreadObjTableInfo,
                                                                    ThreadObjTableInfoSize, &index);
        FillSizeTColField (stackSize,                               ThreadObjTableInfo,
                                                                    ThreadObjTableInfoSize, &index);

        PrintInfo(ThreadObjTableInfo, ThreadObjTableInfoSize);
        lineCount++;
    }
    else
    {
        // If it's not the first time, print a comma.
        if (!IsPrintedNodeFirst)
        {
            printf(",");
        }
        else
        {
            IsPrintedNodeFirst = false;
        }

        bool printed = false;

        printf("[");

        ExportStrToJson   (THREAD_NAME(threadObjRef->name), ThreadObjTableInfo,
                                                          ThreadObjTableInfoSize, &index, &printed);
        ExportBoolToJson  (threadObjRef->isJoinable,      ThreadObjTableInfo,
                                                          ThreadObjTableInfoSize, &index, &printed);
        ExportBoolToJson  ((threadObjRef->state != THREAD_STATE_NEW), ThreadObjTableInfo,
                                                          ThreadObjTableInfoSize, &index, &printed);
        ExportStrToJson   (detachStateStr,                ThreadObjTableInfo,
                                                          ThreadObjTableInfoSize, &index, &printed);
        ExportStrToJson   (schedPolicyStr,                ThreadObjTableInfo,
                                                          ThreadObjTableInfoSize, &index, &printed);
        ExportIntToJson   (schedParam.sched_priority,     ThreadObjTableInfo,
                                                          ThreadObjTableInfoSize, &index, &printed);
        ExportStrToJson   (inheritSchedStr,               ThreadObjTableInfo,
                                                          ThreadObjTableInfoSize, &index, &printed);
        ExportStrToJson   (contentionScopeStr,            ThreadObjTableInfo,
                                                          ThreadObjTableInfoSize, &index, &printed);
        ExportSizeTToJson (guardSize,                     ThreadObjTableInfo,
                                                          ThreadObjTableInfoSize, &index, &printed);
        ExportUint64ToJson(stackAddr[0],                  ThreadObjTableInfo,
                                                          ThreadObjTableInfoSize, &index, &printed);
        ExportSizeTToJson (stackSize,                     ThreadObjTableInfo,
                                                          ThreadObjTableInfoSize, &index, &printed);

        printf("]");
    }

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print timer information to stdout.
 */
//--------------------------------------------------------------------------------------------------
static int PrintTimerInfo
(
    Timer_t* timerRef   ///< [IN] ref to timer to be printed.
)
{
    int lineCount = 0;
    double interval = (double)timerRef->interval.sec + ((double)timerRef->interval.usec / 1000000);
    double expiryTime = (double)timerRef->expiryTime.sec +
                        ((double)timerRef->expiryTime.usec / 1000000);

    // Output timer info
    int index = 0;

    if (!IsOutputJson)
    {
        FillStrColField   (TIMER_NAME(timerRef->name), TimerTableInfo, TimerTableInfoSize, &index);
        FillDoubleColField(interval,              TimerTableInfo, TimerTableInfoSize, &index);
        FillUint32ColField(timerRef->repeatCount, TimerTableInfo, TimerTableInfoSize, &index);
        FillBoolColField  (timerRef->isActive,    TimerTableInfo, TimerTableInfoSize, &index);
        FillDoubleColField(expiryTime,            TimerTableInfo, TimerTableInfoSize, &index);
        FillUint32ColField(timerRef->expiryCount, TimerTableInfo, TimerTableInfoSize, &index);

        PrintInfo(TimerTableInfo, TimerTableInfoSize);
        lineCount++;
    }
    else
    {
        // If it's not the first time, print a comma.
        if (!IsPrintedNodeFirst)
        {
            printf(",");
        }
        else
        {
            IsPrintedNodeFirst = false;
        }

        bool printed = false;

        printf("[");

        ExportStrToJson   (TIMER_NAME(timerRef->name), TimerTableInfo,
                                                  TimerTableInfoSize, &index, &printed);
        ExportDoubleToJson(interval,              TimerTableInfo,
                                                  TimerTableInfoSize, &index, &printed);
        ExportUint32ToJson(timerRef->repeatCount, TimerTableInfo,
                                                  TimerTableInfoSize, &index, &printed);
        ExportBoolToJson  (timerRef->isActive,    TimerTableInfo,
                                                  TimerTableInfoSize, &index, &printed);
        ExportDoubleToJson(expiryTime,            TimerTableInfo,
                                                  TimerTableInfoSize, &index, &printed);
        ExportUint32ToJson(timerRef->expiryCount, TimerTableInfo,
                                                  TimerTableInfoSize, &index, &printed);

        printf("]");
    }

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Helper functions for GetWaitingListThreadNames
 */
//--------------------------------------------------------------------------------------------------
// Given a waiting list link ptr, get a ptr to the thread record
static void* GetMutexThreadRecPtr
(
    le_dls_Link_t* currNodeLinkPtr  ///< [IN] waiting list link ptr.
)
{
    return CONTAINER_OF(currNodeLinkPtr, mutex_ThreadRec_t, waitingListLink);
}

// Given a thread rec ptr, get a ptr to the thread obj
static thread_Obj_t* GetThreadPtrFromMutexLink
(
    void* currNodePtr  ///< [IN] thread record ptr.
)
{
    return CONTAINER_OF(currNodePtr, thread_Obj_t, mutexRec);
}

// Given a waiting list link ptr, get a ptr to the thread record
static void* GetSemThreadRecPtr
(
    le_dls_Link_t* currNodeLinkPtr  ///< [IN] waiting list link ptr.
)
{
    return CONTAINER_OF(currNodeLinkPtr, sem_ThreadRec_t, waitingListLink);
}

// Given a thread rec ptr, get a ptr to the thread obj
static thread_Obj_t* GetThreadPtrFromSemLink
(
    void* currNodePtr    ///< [IN] thread record ptr.
)
{
    return CONTAINER_OF(currNodePtr, thread_Obj_t, semaphoreRec);
}

// Retrieve the waiting list link from a mutex or semaphore thread record.
static le_dls_Link_t GetWaitingListLink
(
    InspType_t inspectType, ///< [IN] What to inspect.
    void* threadRecPtr
)
{
    switch (inspectType)
    {
        case INSPECT_INSP_TYPE_MUTEX:
            return ((mutex_ThreadRec_t*)threadRecPtr)->waitingListLink;
            break;

        case INSPECT_INSP_TYPE_SEMAPHORE:
            return ((sem_ThreadRec_t*)threadRecPtr)->waitingListLink;
            break;

        default:
            INTERNAL_ERR("Failed to get the waiting list link - unexpected inspect type %d.",
                         inspectType);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Given a remote "waiting list" with thread records as members, constrcut an array of thread names
 * that are on the "waiting list".
 */
//--------------------------------------------------------------------------------------------------
static void GetWaitingListThreadNames
(
    InspType_t inspectType,          ///< [IN] What to inspect.
    le_dls_List_t remoteWaitingList, ///< [IN] Waiting list in the remote process.
    char** waitingThreadNames,       ///< [OUT] Array of thread names on the waiting list.
    size_t waitingThreadNamesNum,    ///< [IN] Max number of thread names the array can contain.
    int* threadNameNumPtr            ///< [OUT] Number of thread names written to the array.
)
{
    void* (*getThreadRecPtrFunc)(le_dls_Link_t* currNodeLinkPtr);
    thread_Obj_t* (*getThreadPtrFromLinkFunc)(void* currNodePtr);

    // Generalization of the thread records containing waiting lists.
    typedef union
    {
        mutex_ThreadRec_t m;
        sem_ThreadRec_t s;
    }
    ThreadRec_t;

    size_t threadRecSize;

    switch (inspectType)
    {
        case INSPECT_INSP_TYPE_MUTEX:
            getThreadRecPtrFunc      = GetMutexThreadRecPtr;
            getThreadPtrFromLinkFunc = GetThreadPtrFromMutexLink;
            threadRecSize = sizeof(mutex_ThreadRec_t);
            break;

        case INSPECT_INSP_TYPE_SEMAPHORE:
            getThreadRecPtrFunc      = GetSemThreadRecPtr;
            getThreadPtrFromLinkFunc = GetThreadPtrFromSemLink;
            threadRecSize = sizeof(sem_ThreadRec_t);
            break;

        default:
            INTERNAL_ERR("Failed to get the waiting list link - unexpected inspect type %d.",
                         inspectType);
    }

    RemoteDlsListAccess_t waitingList = {remoteWaitingList, NULL, NULL};
    le_dls_Link_t* currNodeLinkPtr = GetNextDlsLink(&waitingList, NULL);

    void* currNodePtr;
    thread_Obj_t* currThreadPtr;

    ThreadRec_t localThreadRecCopy;
    thread_Obj_t localThreadObjCopy;

#if LE_CONFIG_THREAD_NAMES_ENABLED
    // Clear the thread name.
    memset(localThreadObjCopy.name, 0, sizeof(localThreadObjCopy.name));
#endif

    int i = 0;
    while (currNodeLinkPtr != NULL)
    {
        // From the thread record link ptr on the waiting list, get the associated thread obj ptr.
        currNodePtr = getThreadRecPtrFunc(currNodeLinkPtr);
        currThreadPtr = getThreadPtrFromLinkFunc(currNodePtr);

        // Read the thread obj into the local memory.
        if (TargetReadAddress(PidToInspect, (uintptr_t)currThreadPtr, &localThreadObjCopy,
                              sizeof(localThreadObjCopy)) != LE_OK)
        {
            INTERNAL_ERR(REMOTE_READ_ERR("thread object"));
        }

        // TODO: write accessor functions to do boundary checking, or if ticket 2847 is approved,
        // use Vector.
        if (i >= waitingThreadNamesNum)
        {
            INTERNAL_ERR("Array too small to contain all thread names on the waiting list.");
        }
        // Add the thread name to the array of waiting thread names.
#if LE_CONFIG_THREAD_NAMES_ENABLED
        waitingThreadNames[i] = strndup(localThreadObjCopy.name, sizeof(localThreadObjCopy.name));
#else
        waitingThreadNames[i] = strdup("<omitted>");
#endif
        i++;

        // Get the ptr to the the next node link on the waiting list, by reading the thread record
        // to the local memory first. GetNextDlsLink must operate on a ref to a locally existing link.
        if (TargetReadAddress(PidToInspect, (uintptr_t)currNodePtr,
                              &localThreadRecCopy, threadRecSize) != LE_OK)
        {
            INTERNAL_ERR(REMOTE_READ_ERR("thread record with waiting list"));
        }

        le_dls_Link_t waitingListLink = GetWaitingListLink(inspectType, &localThreadRecCopy);

        currNodeLinkPtr = GetNextDlsLink(&waitingList, &waitingListLink);
    }

    *threadNameNumPtr = i;
}


//--------------------------------------------------------------------------------------------------
/**
 * Given an array of strings, estimate the size needed for a string which is a JSON array consisting
 * of all strings in the input array.
 *
 * @return
 *      Estimated size of the JSON array.
 */
//--------------------------------------------------------------------------------------------------
static int EstimateJsonArraySizeFromStrings
(
    char** stringArray, ///< [IN] Array of strings to construct the json array with.
    int stringNum       ///< [IN] Number of strings in the above array.
)
{
    int waitingThreadJsonArraySize = 0;
    int i;

    for (i = 0; i < stringNum; i++)
    {
        // Plus 3 for the double quotes and comma.
        waitingThreadJsonArraySize += (strlen(stringArray[i]) + 3);
    }
    // For the comma of the last item.
    waitingThreadJsonArraySize -= (stringNum > 0) ? 1 : 0;
    // For the beginning and ending square brackets, and the null terminating char.
    waitingThreadJsonArraySize += 3;

    return waitingThreadJsonArraySize;
}


//--------------------------------------------------------------------------------------------------
/**
 * Given an array of strings, construct a string which is a JSON array consisting of all strings in
 * the input array.
 */
//--------------------------------------------------------------------------------------------------
static void ConstructJsonArrayFromStrings
(
    char** stringArray, ///< [IN] Array of strings to construct the json array with.
    int stringNum,      ///< [IN] Number of strings in the above array.
    char* waitingThreadJsonArray,  ///< [OUT] The resulting string which is a JSON array.
    int waitingThreadJsonArraySize ///< [IN] Size of the resulting string.
)
{
    int strIdx = 0;
    int i;

    strIdx += snprintf((waitingThreadJsonArray + strIdx),
                       (waitingThreadJsonArraySize - strIdx), "[");

    for (i = 0; i < stringNum; i++)
    {
        strIdx += snprintf((waitingThreadJsonArray + strIdx),
                           (waitingThreadJsonArraySize - strIdx), "\"%s\",",
                           stringArray[i]);
    }

    // Delete the last comma, if it exists.
    int deleteComma = 0;
    deleteComma = (stringNum > 0) ? 1 : 0;
    strIdx -= deleteComma;
    strIdx += snprintf((waitingThreadJsonArray + strIdx),
                       (waitingThreadJsonArraySize - strIdx), "]");
}


//--------------------------------------------------------------------------------------------------
/**
 * Print mutex information to stdout.
 */
//--------------------------------------------------------------------------------------------------
static int PrintMutexInfo
(
    Mutex_t* mutexRef   ///< [IN] ref to mutex to be printed.
)
{
    int lineCount = 0;

    #define MAX_THREADS 400 // should be plenty; with an AR7 only 379 threads can be created.
    char* waitingThreadNames[MAX_THREADS] = {0};
    int i = 0;
    GetWaitingListThreadNames(INSPECT_INSP_TYPE_MUTEX, mutexRef->waitingList, waitingThreadNames,
                              MAX_THREADS, &i);

    // Output mutex info
    int index = 0;

    if (!IsOutputJson)
    {
        FillStrColField (MUTEX_NAME(mutexRef->name), MutexTableInfo, MutexTableInfoSize, &index);
        FillIntColField (mutexRef->lockCount,   MutexTableInfo, MutexTableInfoSize, &index);
        FillBoolColField(mutexRef->isRecursive, MutexTableInfo, MutexTableInfoSize, &index);
        FillStrColField (waitingThreadNames[0], MutexTableInfo, MutexTableInfoSize, &index);

        PrintInfo(MutexTableInfo, MutexTableInfoSize);
        lineCount++;

        int j;
        for (j = 1; j < i; j++)
        {
            PrintUnderColumn("WAITING LIST", MutexTableInfo, MutexTableInfoSize,
                             waitingThreadNames[j]);
            lineCount++;
        }
    }
    else
    {
        int waitingThreadJsonArraySize = EstimateJsonArraySizeFromStrings(waitingThreadNames, i);
        char waitingThreadJsonArray[waitingThreadJsonArraySize];
        ConstructJsonArrayFromStrings(waitingThreadNames, i, waitingThreadJsonArray,
                                      waitingThreadJsonArraySize);

        // If it's not the first time, print a comma.
        if (!IsPrintedNodeFirst)
        {
            printf(",");
        }
        else
        {
            IsPrintedNodeFirst = false;
        }

        bool printed = false;

        printf("[");

        ExportStrToJson  (MUTEX_NAME(mutexRef->name), MutexTableInfo,
                                                  MutexTableInfoSize, &index, &printed);
        ExportIntToJson  (mutexRef->lockCount,    MutexTableInfo,
                                                  MutexTableInfoSize, &index, &printed);
        ExportBoolToJson (mutexRef->isRecursive,  MutexTableInfo,
                                                  MutexTableInfoSize, &index, &printed);
        ExportArrayToJson(waitingThreadJsonArray, MutexTableInfo,
                                                  MutexTableInfoSize, &index, &printed);

        printf("]");
    }

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print semaphore information to stdout.
 */
//--------------------------------------------------------------------------------------------------
static int PrintSemaphoreInfo
(
    Semaphore_t* semaphoreRef   ///< [IN] ref to semaphore to be printed.
)
{
    int lineCount = 0;

    #define MAX_THREADS 400 // should be plenty; with an AR7 only 379 threads can be created.
    char* waitingThreadNames[MAX_THREADS] = {0};
    int i = 0;
    GetWaitingListThreadNames(INSPECT_INSP_TYPE_SEMAPHORE, semaphoreRef->waitingList,
                              waitingThreadNames, MAX_THREADS, &i);

    // Output semaphore info
    int index = 0;

    if (!IsOutputJson)
    {
        FillStrColField(SEM_NAME(semaphoreRef->nameStr), SemaphoreTableInfo,
            SemaphoreTableInfoSize, &index);
        FillStrColField(waitingThreadNames[0], SemaphoreTableInfo, SemaphoreTableInfoSize, &index);

        PrintInfo(SemaphoreTableInfo, SemaphoreTableInfoSize);
        lineCount++;

        int j;
        for (j = 1; j < i; j++)
        {
            PrintUnderColumn("WAITING LIST", SemaphoreTableInfo, SemaphoreTableInfoSize,
                             waitingThreadNames[j]);
            lineCount++;
        }
    }
    else
    {
        int waitingThreadJsonArraySize = EstimateJsonArraySizeFromStrings(waitingThreadNames, i);
        char waitingThreadJsonArray[waitingThreadJsonArraySize];
        ConstructJsonArrayFromStrings(waitingThreadNames, i, waitingThreadJsonArray,
                                      waitingThreadJsonArraySize);

        // If it's not the first time, print a comma.
        if (!IsPrintedNodeFirst)
        {
            printf(",");
        }
        else
        {
            IsPrintedNodeFirst = false;
        }

        bool printed = false;

        printf("[");

        ExportStrToJson  (SEM_NAME(semaphoreRef->nameStr), SemaphoreTableInfo,
                                                  SemaphoreTableInfoSize, &index, &printed);
        ExportArrayToJson(waitingThreadJsonArray, SemaphoreTableInfo,
                                                  SemaphoreTableInfoSize, &index, &printed);

        printf("]");
    }

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Look up the thread name associated with the thread object safe ref being passed in. If there's no
 * match, the out buffer is emptied.
 */
//--------------------------------------------------------------------------------------------------
static void LookupThreadName
(
    size_t threadObjSafeRefAddr,   ///< [IN] thread obj safe ref used to look up thread name.
    char* threadObjNameBuffer,     ///< [OUT] out buffer to stored the thread name.
    size_t threadObjNameBufferSize ///< [IN] size of the out buffer.
)
{
    ThreadObjIter_Ref_t threadObjIterRef = CreateThreadObjIter();
    thread_Obj_t* threadObjRef = NULL;

    do
    {
        threadObjRef = GetNextThreadObj(threadObjIterRef);
        if (threadObjRef)
        {
            if (threadObjSafeRefAddr == (size_t)threadObjRef->safeRef)
            {
                // copy thread name to the out buffer
                strncpy(threadObjNameBuffer, THREAD_NAME(threadObjRef->name),
                    threadObjNameBufferSize);
                return;
            }
        }
    }
    while (threadObjRef != NULL);

    // thread name not found; zero-out the out buffer.
    memset(threadObjNameBuffer, 0, threadObjNameBufferSize);

    // delete the thread object iterator ref.
    le_mem_Release(threadObjIterRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Print service object information to stdout.
 */
//--------------------------------------------------------------------------------------------------
static int PrintServiceObjInfo
(
    msgInterface_UnixService_t* serviceObjRef   ///< [IN] ref to service obj to be printed.
)
{
    int lineCount = 0;

    // Retrieve the protocol object.
    msgProtocol_Protocol_t protocol;

    // Read the protocol object into our own memory.
    if (TargetReadAddress(PidToInspect, (uintptr_t)serviceObjRef->interface.id.protocolRef, &protocol,
                          sizeof(protocol)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("protocol object"));
    }

    // Convert the service state to a meaningful string.
    char* serviceStateStr = DefnToStr(serviceObjRef->state, ServiceStateTbl, ServiceStateTblSize);

    // Retrieve the thread name
    char threadName[MAX_THREAD_NAME_SIZE] = {0};
    LookupThreadName((size_t)serviceObjRef->serverThread, threadName, MAX_THREAD_NAME_SIZE);

    // Output service object info
    int index = 0;

    if (!IsOutputJson)
    {
        FillStrColField  (serviceObjRef->interface.id.name, ServiceObjTableInfo,
                                                            ServiceObjTableInfoSize, &index);
        FillStrColField  (serviceStateStr,                  ServiceObjTableInfo,
                                                            ServiceObjTableInfoSize, &index);
        FillStrColField  (threadName,                       ServiceObjTableInfo,
                                                            ServiceObjTableInfoSize, &index);
        FillStrColField  (protocol.id,                      ServiceObjTableInfo,
                                                            ServiceObjTableInfoSize, &index);
        FillSizeTColField(protocol.maxPayloadSize,          ServiceObjTableInfo,
                                                            ServiceObjTableInfoSize, &index);
        FillIntColField  (serviceObjRef->directorySocketFd, ServiceObjTableInfo,
                                                            ServiceObjTableInfoSize, &index);

        PrintInfo(ServiceObjTableInfo, ServiceObjTableInfoSize);
        lineCount++;
    }
    else
    {
        // If it's not the first time, print a comma.
        if (!IsPrintedNodeFirst)
        {
            printf(",");
        }
        else
        {
            IsPrintedNodeFirst = false;
        }

        bool printed = false;

        printf("[");

        ExportStrToJson  (serviceObjRef->interface.id.name, ServiceObjTableInfo,
                                                         ServiceObjTableInfoSize, &index, &printed);
        ExportStrToJson  (serviceStateStr,               ServiceObjTableInfo,
                                                         ServiceObjTableInfoSize, &index, &printed);
        ExportStrToJson  (threadName,                    ServiceObjTableInfo,
                                                         ServiceObjTableInfoSize, &index, &printed);
        ExportStrToJson  (protocol.id,                   ServiceObjTableInfo,
                                                         ServiceObjTableInfoSize, &index, &printed);
        ExportSizeTToJson(protocol.maxPayloadSize,       ServiceObjTableInfo,
                                                         ServiceObjTableInfoSize, &index, &printed);
        ExportIntToJson  (serviceObjRef->directorySocketFd, ServiceObjTableInfo,
                                                         ServiceObjTableInfoSize, &index, &printed);

        printf("]");
    }

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print client interface object information to stdout.
 */
//--------------------------------------------------------------------------------------------------
static int PrintClientObjInfo
(
    msgInterface_ClientInterface_t* clientObjRef   ///< [IN] ref to client i/f obj to be printed.
)
{
    int lineCount = 0;

    // Retrieve the protocol object. Read the protocol object into our own memory.
    msgProtocol_Protocol_t protocol;
    if (TargetReadAddress(PidToInspect, (uintptr_t)clientObjRef->interface.id.protocolRef, &protocol,
                          sizeof(protocol)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("protocol object"));
    }

    // Output client object info
    int index = 0;

    if (!IsOutputJson)
    {
        FillStrColField  (clientObjRef->interface.id.name, ClientObjTableInfo,
                                                           ClientObjTableInfoSize, &index);
        FillStrColField  (protocol.id,                     ClientObjTableInfo,
                                                           ClientObjTableInfoSize, &index);
        FillSizeTColField(protocol.maxPayloadSize,         ClientObjTableInfo,
                                                           ClientObjTableInfoSize, &index);

        PrintInfo(ClientObjTableInfo, ClientObjTableInfoSize);
        lineCount++;
    }
    else
    {
        // If it's not the first time, print a comma.
        if (!IsPrintedNodeFirst)
        {
            printf(",");
        }
        else
        {
            IsPrintedNodeFirst = false;
        }

        bool printed = false;

        printf("[");

        ExportStrToJson  (clientObjRef->interface.id.name, ClientObjTableInfo,
                                                          ClientObjTableInfoSize, &index, &printed);
        ExportStrToJson  (protocol.id,                    ClientObjTableInfo,
                                                          ClientObjTableInfoSize, &index, &printed);
        ExportSizeTToJson(protocol.maxPayloadSize,        ClientObjTableInfo,
                                                          ClientObjTableInfoSize, &index, &printed);

        printf("]");
    }

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print session object information to stdout.
 */
//--------------------------------------------------------------------------------------------------
static int PrintSessionObjInfo
(
    msgSession_UnixSession_t* sessionObjRef ///< [IN] ref to session obj to be printed.
)
{
    int lineCount = 0;

    // Convert the session state to a meaningful string.
    char* sessionStateStr = DefnToStr(sessionObjRef->state, SessionStateTbl, SessionStateTblSize);

    // Retrieve the interface object. Read the interface object into our own memory.
    msgInterface_Interface_t interface;
    if (TargetReadAddress(PidToInspect, (uintptr_t)sessionObjRef->interfaceRef, &interface,
                          sizeof(interface)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("interface object"));
    }

    // Retrieve the thread name
    char threadName[MAX_THREAD_NAME_SIZE] = {0};
    LookupThreadName((size_t)sessionObjRef->threadRef, threadName, MAX_THREAD_NAME_SIZE);

    // Output session object info
    int index = 0;

    if (!IsOutputJson)
    {
        FillStrColField(interface.id.name,       SessionObjTableInfo,
                                                 SessionObjTableInfoSize, &index);
        FillStrColField(sessionStateStr,         SessionObjTableInfo,
                                                 SessionObjTableInfoSize, &index);
        FillStrColField(threadName,              SessionObjTableInfo,
                                                 SessionObjTableInfoSize, &index);
        FillIntColField(sessionObjRef->socketFd, SessionObjTableInfo,
                                                 SessionObjTableInfoSize, &index);

        PrintInfo(SessionObjTableInfo, SessionObjTableInfoSize);
        lineCount++;
    }
    else
    {
        // If it's not the first time, print a comma.
        if (!IsPrintedNodeFirst)
        {
            printf(",");
        }
        else
        {
            IsPrintedNodeFirst = false;
        }

        bool printed = false;

        printf("[");

        ExportStrToJson(interface.id.name,       SessionObjTableInfo,
                                                 SessionObjTableInfoSize, &index, &printed);
        ExportStrToJson(sessionStateStr,         SessionObjTableInfo,
                                                 SessionObjTableInfoSize, &index, &printed);
        ExportStrToJson(threadName,              SessionObjTableInfo,
                                                 SessionObjTableInfoSize, &index, &printed);
        ExportIntToJson(sessionObjRef->socketFd, SessionObjTableInfo,
                                                 SessionObjTableInfoSize, &index, &printed);

        printf("]");
    }

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function prototype needed by InspectEndHandling.
 */
//--------------------------------------------------------------------------------------------------
static void RefreshTimerHandler
(
    le_timer_Ref_t timerRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Performs actions when an inspection ends depending on how it ends.
 */
//--------------------------------------------------------------------------------------------------
static int InspectEndHandling
(
    InspectEndStatus_t endStatus ///< [IN] How an inspection ended.
)
{
    int lineCount = 0;

    if (!IsOutputJson)
    {
        if (endStatus == INSPECT_INTERRUPTED)
        {
            printf(">>> Detected list changes. Stopping inspection. <<<\n");
            lineCount++;
        }
    }
    else
    {
        // Print the end of "Data".
        printf("],");

        if (endStatus == INSPECT_INTERRUPTED)
        {
            printf("\"Interrupted\":true");
        }
        else
        {
            printf("\"Interrupted\":false");
        }

        // Print the end of the "main" JSON object.
        printf("}\n");
    }

    // The last line of the current run of inspection has finished, so it's a good place to
    // flush the write buffer on stdout. This is important for redirecting the output to a
    // log file, so that the end of an inspection is written to the log as soon as it
    // happens.
    fflush(stdout);

    // If Inspect is set to repeat periodically, configure the repeat interval.
    if (IsFollowing)
    {
        // Reset this boolean for the next round.
        IsPrintedNodeFirst = true;

        le_clk_Time_t refreshInterval;

        switch (endStatus)
        {
            case INSPECT_SUCCESS:
                refreshInterval.sec = RefreshInterval;
                refreshInterval.usec = 0;
                break;

            case INSPECT_INTERRUPTED:
                refreshInterval.sec = 0;
                refreshInterval.usec = DEFAULT_RETRY_INTERVAL;
                break;

            default:
                INTERNAL_ERR("Invalid end status.");
        }

        // Set up the refresh timer.
        refreshTimer = le_timer_Create("RefreshTimer");

        INTERNAL_ERR_IF(le_timer_SetHandler(refreshTimer, RefreshTimerHandler) != LE_OK,
                        "Could not set timer handler.\n");

        INTERNAL_ERR_IF(le_timer_SetInterval(refreshTimer, refreshInterval) != LE_OK,
                        "Could not set refresh time.\n");

        // Start the refresh timer.
        INTERNAL_ERR_IF(le_timer_Start(refreshTimer) != LE_OK,
                        "Could not start refresh timer.\n");
    }

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Performs the specified inspection for the specified process. Prints the results to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void InspectFunc
(
    InspType_t inspectType ///< [IN] What to inspect.
)
{
    // function prototype for the CreateXXXIter family.
    typedef void* (*CreateIterFunc_t)(void);
    // function prototype for the GetXXXListChgCnt family.
    typedef size_t (*GetListChgCntFunc_t)(void* iterRef);
    // function prototype for the GetNextXXX family.
    typedef void* (*GetNextNodeFunc_t)(void* iterRef);
    // Function prototype for the PrintXXXInfo family.
    typedef int (*PrintNodeInfoFunc_t)(void* nodeRef);

    CreateIterFunc_t createIterFunc;
    GetListChgCntFunc_t getListChgCntFunc;
    GetNextNodeFunc_t getNextNodeFunc;
    PrintNodeInfoFunc_t printNodeInfoFunc;

    // assigns the appropriate set of functions according to the inspection type.
    switch (inspectType)
    {
        case INSPECT_INSP_TYPE_MEM_POOL:
            createIterFunc    = (CreateIterFunc_t)    CreateMemPoolIter;
            getListChgCntFunc = (GetListChgCntFunc_t) GetMemPoolListChgCnt;
            getNextNodeFunc   = (GetNextNodeFunc_t)   GetNextMemPool;
            printNodeInfoFunc = (PrintNodeInfoFunc_t) PrintMemPoolInfo;
            break;

        case INSPECT_INSP_TYPE_THREAD_OBJ:
            createIterFunc    = (CreateIterFunc_t)    CreateThreadObjIter;
            getListChgCntFunc = (GetListChgCntFunc_t) GetThreadObjListChgCnt;
            getNextNodeFunc   = (GetNextNodeFunc_t)   GetNextThreadObj;
            printNodeInfoFunc = (PrintNodeInfoFunc_t) PrintThreadObjInfo;
            break;

        case INSPECT_INSP_TYPE_TIMER:
            createIterFunc    = (CreateIterFunc_t)    CreateTimerIter;
            getListChgCntFunc = (GetListChgCntFunc_t) GetThreadMemberObjListChgCnt;
            getNextNodeFunc   = (GetNextNodeFunc_t)   GetNextTimer;
            printNodeInfoFunc = (PrintNodeInfoFunc_t) PrintTimerInfo;
            break;

        case INSPECT_INSP_TYPE_MUTEX:
            createIterFunc    = (CreateIterFunc_t)    CreateMutexIter;
            getListChgCntFunc = (GetListChgCntFunc_t) GetThreadMemberObjListChgCnt;
            getNextNodeFunc   = (GetNextNodeFunc_t)   GetNextMutex;
            printNodeInfoFunc = (PrintNodeInfoFunc_t) PrintMutexInfo;
            break;

        case INSPECT_INSP_TYPE_SEMAPHORE:
            createIterFunc    = (CreateIterFunc_t)    CreateSemaphoreIter;
            getListChgCntFunc = (GetListChgCntFunc_t) GetThreadMemberObjListChgCnt;
            getNextNodeFunc   = (GetNextNodeFunc_t)   GetNextSemaphore;
            printNodeInfoFunc = (PrintNodeInfoFunc_t) PrintSemaphoreInfo;
            break;

        case INSPECT_INSP_TYPE_IPC_SERVERS:
            createIterFunc    = (CreateIterFunc_t)    CreateServiceObjIter;
            getListChgCntFunc = (GetListChgCntFunc_t) GetInterfaceObjMapChgCnt;
            getNextNodeFunc   = (GetNextNodeFunc_t)   GetNextServiceObj;
            printNodeInfoFunc = (PrintNodeInfoFunc_t) PrintServiceObjInfo;
            break;

        case INSPECT_INSP_TYPE_IPC_CLIENTS:
            createIterFunc    = (CreateIterFunc_t)    CreateClientObjIter;
            getListChgCntFunc = (GetListChgCntFunc_t) GetInterfaceObjMapChgCnt;
            getNextNodeFunc   = (GetNextNodeFunc_t)   GetNextClientObj;
            printNodeInfoFunc = (PrintNodeInfoFunc_t) PrintClientObjInfo;
            break;

        case INSPECT_INSP_TYPE_IPC_SERVERS_SESSIONS:
        case INSPECT_INSP_TYPE_IPC_CLIENTS_SESSIONS:
            createIterFunc    = (CreateIterFunc_t)    CreateSessionObjIter;
            getListChgCntFunc = (GetListChgCntFunc_t) GetSessionListChgCnt;
            getNextNodeFunc   = (GetNextNodeFunc_t)   GetNextSessionObj;
            printNodeInfoFunc = (PrintNodeInfoFunc_t) PrintSessionObjInfo;
            break;

        default:
            INTERNAL_ERR("unexpected inspect type %d.", inspectType);
    }

    // Create an iterator.
    void* iterRef = createIterFunc();

    static int lineCount = 0;

    // Print header information.
    if (!IsOutputJson)
    {
        printf("%c[1G", ESCAPE_CHAR);             // Move cursor to the column 1.
        printf("%c[%dA", ESCAPE_CHAR, lineCount); // Move cursor up to the top of the table.
        printf("%c[0J", ESCAPE_CHAR);             // Clear Screen.
    }

    lineCount += PrintInspectHeader();


    // Iterate through the list of nodes.
    size_t initialChangeCount = getListChgCntFunc(iterRef);
    size_t currentChangeCount;
    void* nodeRef = NULL;

    do
    {
        nodeRef = getNextNodeFunc(iterRef);

        if (nodeRef != NULL)
        {
            lineCount += printNodeInfoFunc(nodeRef);
        }

        currentChangeCount = getListChgCntFunc(iterRef);
    }
    // Access the next node only if the current node is not NULL and there has been no changes to
    // the node list.
    while ((nodeRef != NULL) && (currentChangeCount == initialChangeCount));

    // If the loop terminated because the next node is NULL and there has been no changes to the
    // node list, then we can delcare the end of list has been reached.
    if ((nodeRef == NULL) && (currentChangeCount == initialChangeCount))
    {
        lineCount += InspectEndHandling(INSPECT_SUCCESS);
    }
    // Detected changes to the node list.
    else
    {
        lineCount += InspectEndHandling(INSPECT_INTERRUPTED);
    }

    // Note that InspectFunc is called multiple times when the "interval mode" is on, so don't
    // close the fd "FdProcMem". Let the OS handle the cleanup.
    le_mem_Release(iterRef);

    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * Refresh timer handler.
 */
//--------------------------------------------------------------------------------------------------
static void RefreshTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    TargetStop(PidToInspect);

    // Perform the inspection.
    InspectFunc(InspectType);

    TargetStart(PidToInspect);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function called when a signal is received to stop tracing
 */
//--------------------------------------------------------------------------------------------------
static void ExitEventHandler
(
    int sigNum
)
{
    TargetStop(PidToInspect);
    TargetDetach(PidToInspect);

    exit(0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function called by command line argument scanner when the pid argument is found.
 **/
//--------------------------------------------------------------------------------------------------
static void PidArgHandler
(
    const char* pidStr
)
{
    int pid;
    le_result_t result = le_utf8_ParseInt(&pid, pidStr);

    if ((result == LE_OK) && (pid > 0))
    {
        PidToInspect = pid;
    }
    else
    {
        fprintf(stderr, "Invalid PID (%s).\n", pidStr);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * IPC sessions argument handler.
 */
//--------------------------------------------------------------------------------------------------
static void IpcSessionArgHandler
(
    const char* sessionsArg
)
{
    if (strcmp(sessionsArg, "sessions") == 0)
    {
        switch (InspectType)
        {
            case INSPECT_INSP_TYPE_IPC_SERVERS:
                InspectType = INSPECT_INSP_TYPE_IPC_SERVERS_SESSIONS;
                break;

            case INSPECT_INSP_TYPE_IPC_CLIENTS:
                InspectType = INSPECT_INSP_TYPE_IPC_CLIENTS_SESSIONS;
                break;

            default:
                INTERNAL_ERR("unexpected inspect type %d.", InspectType);
        }

        // Handle the next argument which should be PID.
        le_arg_AddPositionalCallback(PidArgHandler);
    }
    else
    {
        // Assume this argument is PID.
        PidArgHandler(sessionsArg);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * IPC interface type handler.
 */
//--------------------------------------------------------------------------------------------------
static void IpcInterfaceTypeHandler
(
    const char* interfaceType
)
{
    if (strcmp(interfaceType, "servers") == 0)
    {
        InspectType = INSPECT_INSP_TYPE_IPC_SERVERS;
    }
    else if (strcmp(interfaceType, "clients") == 0)
    {
        InspectType = INSPECT_INSP_TYPE_IPC_CLIENTS;
    }
    else
    {
        fprintf(stderr, "Invalid interface type '%s'.\n", interfaceType);
        exit(EXIT_FAILURE);
    }

    // Handle the optional "sessions" argument.
    le_arg_AddPositionalCallback(IpcSessionArgHandler);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function called by command line argument scanner when the command argument is found.
 **/
//--------------------------------------------------------------------------------------------------
static void CommandArgHandler
(
    const char* command
)
{
    if (strcmp(command, "pools") == 0)
    {
        InspectType = INSPECT_INSP_TYPE_MEM_POOL;
    }
    else if (strcmp(command, "threads") == 0)
    {
        InspectType = INSPECT_INSP_TYPE_THREAD_OBJ;
    }
    else if (strcmp(command, "timers") == 0)
    {
        InspectType = INSPECT_INSP_TYPE_TIMER;
    }
    else if (strcmp(command, "mutexes") == 0)
    {
        InspectType = INSPECT_INSP_TYPE_MUTEX;
    }
    else if (strcmp(command, "semaphores") == 0)
    {
        InspectType = INSPECT_INSP_TYPE_SEMAPHORE;
    }
    else if (strcmp(command, "ipc") == 0)
    {
        le_arg_AddPositionalCallback(IpcInterfaceTypeHandler);
    }
    else
    {
        fprintf(stderr, "Invalid command '%s'.\n", command);
        exit(EXIT_FAILURE);
    }

    if (strcmp(command, "ipc") != 0)
    {
        le_arg_AddPositionalCallback(PidArgHandler);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function called by command line argument scanner when the -f or --interval= option is given.
 **/
//--------------------------------------------------------------------------------------------------
static void FollowOptionCallback
(
    int value
)
{
    if (value <= 0)
    {
        fprintf(stderr,
                "Interval value must be a positive integer. "
                    " Using the default interval %d seconds.\n",
                DEFAULT_REFRESH_INTERVAL);

        value = DEFAULT_REFRESH_INTERVAL;
    }

    RefreshInterval = value;

    IsFollowing = true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function called by command line argument scanner when the --format= option is given.
 **/
//--------------------------------------------------------------------------------------------------
static void FormatOptionCallback
(
    const char* format
)
{
    if (strcmp(format, "json") == 0)
    {
        IsOutputJson = true;
    }
    else
    {
        fprintf(stderr, "Bad format specifier, '%s'.\n", format);
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a memory pool for the iterators depending on the inspect type.
 **/
//--------------------------------------------------------------------------------------------------
static void InitIteratorPool
(
    InspType_t inspectType ///< [IN] What to inspect.
)
{
    size_t size;

    switch (inspectType)
    {
        case INSPECT_INSP_TYPE_MEM_POOL:
            size = sizeof(MemPoolIter_t);
            break;

        case INSPECT_INSP_TYPE_THREAD_OBJ:
            size = sizeof(ThreadObjIter_t);
            break;

        case INSPECT_INSP_TYPE_TIMER:
            size = sizeof(TimerIter_t);
            break;

        case INSPECT_INSP_TYPE_MUTEX:
            size = sizeof(MutexIter_t);
            break;

        case INSPECT_INSP_TYPE_SEMAPHORE:
            size = sizeof(SemaphoreIter_t);
            break;

        case INSPECT_INSP_TYPE_IPC_SERVERS:
            // Make the block size big enough to accomodate either one.
            // Technically a little wasteful.
            size = sizeof(ThreadObjIter_t) > sizeof(ServiceObjIter_t) ?
                   sizeof(ThreadObjIter_t) : sizeof(ServiceObjIter_t);
            break;

        case INSPECT_INSP_TYPE_IPC_CLIENTS:
            size = sizeof(ClientObjIter_t);
            break;

        case INSPECT_INSP_TYPE_IPC_SERVERS_SESSIONS:
        case INSPECT_INSP_TYPE_IPC_CLIENTS_SESSIONS:
            size = sizeof(ThreadObjIter_t) > sizeof(SessionObjIter_t) ?
                   sizeof(ThreadObjIter_t) : sizeof(SessionObjIter_t);
            break;

        default:
            INTERNAL_ERR("unexpected inspect type %d.", inspectType);
    }

    IteratorPool = le_mem_CreatePool("Iterators", size);
}


//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // The command-line has a command string followed by a PID.
    le_arg_AddPositionalCallback(CommandArgHandler);

    // --help option causes everything else to be ignored, prints help, and exits.
    le_arg_SetFlagCallback(PrintHelp, NULL, "help");

    // -f option starts "following" (periodic updates until the program is terminated).
    le_arg_SetFlagVar(&IsFollowing, "f", NULL);

    // -v option prints in verbose mode.
    le_arg_SetFlagVar(&IsVerbose, "v", NULL);

    // --interval=N option specifies the update period (implies -f).
    le_arg_SetIntCallback(FollowOptionCallback, NULL, "interval");

    // --format=json option outputs data to the specified file in JSON format.
    le_arg_SetStringCallback(FormatOptionCallback, NULL, "format");

    le_arg_Scan();

    // Create a memory pool for iterators.
    InitIteratorPool(InspectType);

    TargetAttach(PidToInspect);

    InitDisplay(InspectType);

    TargetStop(PidToInspect);

    // Start the inspection.
    InspectFunc(InspectType);

    if (!IsFollowing)
    {
        TargetDetach(PidToInspect);
        exit(EXIT_SUCCESS);
    }
    else
    {
        TargetStart(PidToInspect);

        // Register for SIGTERM so we can detach from process
        le_sig_Block(SIGTERM);
        le_sig_Block(SIGHUP);

        le_sig_SetEventHandler(SIGTERM, ExitEventHandler);
        le_sig_SetEventHandler(SIGHUP, ExitEventHandler);
    }
}
