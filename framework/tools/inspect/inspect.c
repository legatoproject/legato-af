//--------------------------------------------------------------------------------------------------
/** @file inspect.c
 *
 * Legato inspection tool used to inspect Legato structures such as memory pools, timers, threads,
 * mutexes, etc. in running processes.
 *
 * Must be run as root.
 *
 * @todo Only supports memory pools right now.  Add support for other timers, threads, etc.
 *
 * @todo Add inspect by process name.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "spy.h"
#include "mem.h"
#include "thread.h"
#include "limit.h"
#include "addr.h"
#include "fileDescriptor.h"


//--------------------------------------------------------------------------------------------------
/**
 * Objects of these types are used to refer to lists of memory pools, thread objects, timers,
 * mutexes, and semaphores. They can be used to iterate over those lists in a remote process.
 */
//--------------------------------------------------------------------------------------------------
typedef struct MemPoolIter* MemPoolIter_Ref_t;
typedef struct ThreadObjIter*  ThreadObjIter_Ref_t;
typedef struct TimerIter*  TimerIter_Ref_t;
typedef struct MutexIter* MutexIter_Ref_t;
typedef struct SemaphoreIter* SemaphoreIter_Ref_t;
typedef struct ThreadMemberObjIter* ThreadMemberObjIter_Ref_t;


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
    INSPECT_INSP_TYPE_SEMAPHORE
}
InspType_t;


//--------------------------------------------------------------------------------------------------
/**
 * Object containing items necessary for accessing a list in the remote process.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_List_t List;         ///< The list in the remote process.
    size_t* ListChgCntRef;      ///< Change counter for the remote list.
    le_dls_Link_t* headLinkPtr; ///< Pointer to the first link.
}
RemoteListAccess_t;


//--------------------------------------------------------------------------------------------------
/**
 * Iterator objects for stepping through the list of memory pools, thread objects, timers, mutexes,
 * and semaphores in a remote process.
 */
//--------------------------------------------------------------------------------------------------
typedef struct MemPoolIter
{
    pid_t pid;                      ///< PID of the process this iterator is for.
    int procMemFd;                  ///< File descriptor to the remote process' /proc/pid/mem file.
    RemoteListAccess_t memPoolList; ///< Memory pool list in the remote process.
    MemPool_t currMemPool;          ///< Current memory pool from the list.
}
MemPoolIter_t;

typedef struct ThreadObjIter
{
    pid_t pid;
    int procMemFd;
    RemoteListAccess_t threadObjList; ///< Thread object list in the remote process.
    ThreadObj_t currThreadObj;        ///< Current thread object from the list.
}
ThreadObjIter_t;

typedef struct TimerIter
{
    pid_t pid;
    int procMemFd;
    RemoteListAccess_t threadObjList;
    RemoteListAccess_t timerList;     ///< Timer list for the current thread in the remote process.
    ThreadObj_t currThreadObj;
    Timer_t currTimer;                ///< Current timer from the list.
}
TimerIter_t;

typedef struct MutexIter
{
    pid_t pid;
    int procMemFd;
    RemoteListAccess_t threadObjList;
    RemoteListAccess_t mutexList;     ///< Mutexe list for the current thread in the remote process.
    ThreadObj_t currThreadObj;
    Mutex_t currMutex;                ///< Current mutex from the list.
}
MutexIter_t;

typedef struct SemaphoreIter
{
    pid_t pid;
    int procMemFd;
    RemoteListAccess_t threadObjList;
    RemoteListAccess_t semaphoreList; ///< This is a dummy, since there's no semaphore list.
    ThreadObj_t currThreadObj;
    Semaphore_t currSemaphore;        ///< Current semaphore from the list.
}
SemaphoreIter_t;

// Type describing the commonalities of the thread memeber objects - namely timer, mutex, and
// semaphore.
typedef struct ThreadMemberObjIter
{
    pid_t pid;
    int procMemFd;
    RemoteListAccess_t threadObjList;
    RemoteListAccess_t threadMemberObjList;
    ThreadObj_t currThreadObj;
}
ThreadMemberObjIter_t;


//--------------------------------------------------------------------------------------------------
/**
 * Local memory pools that are used for allocating inspection object iterators.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t MemPoolIteratorPool;
static le_mem_PoolRef_t ThreadObjIteratorPool;
static le_mem_PoolRef_t TimerIteratorPool;
static le_mem_PoolRef_t MutexIteratorPool;
static le_mem_PoolRef_t SemaphoreIteratorPool;


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
//TODO: use this static variable for pid instead of requiring pid as function params
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
static off_t GetRemoteAddress
(
    pid_t pid,              ///< [IN] Remote process to to get the address for.
    void* localAddrPtr      ///< [IN] Local address to get the offset with.
)
{
    // Get the address of our framework library.
    off_t libAddr;
    if (addr_GetLibDataSection(0, "liblegato.so", &libAddr) != LE_OK)
    {
        INTERNAL_ERR("Can't find our framework library address.");
    }

    // Calculate the offset address of the local address by subtracting it by the start of our
    // own framwork library address.
    off_t offset = (off_t)(localAddrPtr) - libAddr;

    // Get the address of the framework library in the remote process.
    if (addr_GetLibDataSection(pid, "liblegato.so", &libAddr) != LE_OK)
    {
        INTERNAL_ERR("Can't find address of the framework library in the remote process.");
    }

    // Calculate the process-under-inspection's counterpart address to the local address  by adding
    // the offset to the start of their framework library address.
    return (libAddr + offset);
}


//--------------------------------------------------------------------------------------------------
/**
 * Opens the /proc/<PID>/mem file for the specified pid and returns its fd.
 *
 * @return
 *      The fd of the "mem" file opened.
 */
//--------------------------------------------------------------------------------------------------
static int OpenProcMemFile
(
    pid_t pid ///< [IN] The pid to open the "mem" file for.
)
{
    // Build the path to the mem file for the process to inspect.
    char memFilePath[LIMIT_MAX_PATH_BYTES];
    int snprintSize = snprintf(memFilePath, sizeof(memFilePath), "/proc/%d/mem", pid);

    if (snprintSize >= sizeof(memFilePath))
    {
        INTERNAL_ERR("Path is too long '%s'.", memFilePath);
    }
    else if (snprintSize < 0)
    {
        INTERNAL_ERR("snprintf encoding error.");
    }

    // Open the mem file for the specified process.
    int fd = open(memFilePath, O_RDONLY);

    if (fd == -1)
    {
        fprintf(stderr, "Could not open %s.  %m.\n", memFilePath);
        exit(EXIT_FAILURE);
    }

    return fd;
}


// TODO: CreateMemPoolIter and CreateThreadObjIter can probably be combined just like
// CreateThreadMemberObjIter
//--------------------------------------------------------------------------------------------------
/**
 * Creates an iterator that can be used to iterate over the list of available memory pools for a
 * specific process.
 *
 * @note
 *      The specified pid must be greater than zero.
 *
 *      The calling process must be root or have appropriate capabilities for this function and all
 *      subsequent operations on the iterator to succeed.
 *
 * @return
 *      An iterator to the list of memory pools for the specified process.
 */
//--------------------------------------------------------------------------------------------------
static MemPoolIter_Ref_t CreateMemPoolIter
(
    pid_t pid ///< [IN] The process to get the iterator for.
)
{
    int fd = OpenProcMemFile(pid);

    // Get the address offset of the list of memory pools for the process to inspect.
    off_t listAddrOffset = GetRemoteAddress(pid, spy_GetListOfPools());

    // Get the address offset of the list of memory pools change counter for the process to inspect.
    off_t listChgCntAddrOffset = GetRemoteAddress(pid, spy_GetListOfPoolsChgCntRef());

    // Create the iterator.
    MemPoolIter_t* iteratorPtr = le_mem_ForceAlloc(MemPoolIteratorPool);
    iteratorPtr->procMemFd = fd;
    iteratorPtr->pid = pid;
    iteratorPtr->memPoolList.headLinkPtr = NULL;

    // Get the List for the process-under-inspection.
    if (fd_ReadFromOffset(fd, listAddrOffset, &(iteratorPtr->memPoolList.List),
                             sizeof(iteratorPtr->memPoolList.List)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("mempool list"));
    }

    // Get the ListChgCntRef for the process-under-inspection.
    if (fd_ReadFromOffset(fd, listChgCntAddrOffset, &(iteratorPtr->memPoolList.ListChgCntRef),
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
    pid_t pid ///< [IN] The process to get the iterator for.
)
{
    int fd = OpenProcMemFile(pid);

    // Get the address offset of the list of thread objs for the process to inspect.
    off_t listAddrOffset = GetRemoteAddress(pid, spy_GetListOfThreadObj());

    // Get the address offset of the list of thread objs change counter for the process to inspect.
    off_t listChgCntAddrOffset = GetRemoteAddress(pid, spy_GetListOfThreadObjsChgCntRef());

    // Create the iterator.
    ThreadObjIter_t* iteratorPtr = le_mem_ForceAlloc(ThreadObjIteratorPool);
    iteratorPtr->procMemFd = fd;
    iteratorPtr->pid = pid;
    iteratorPtr->threadObjList.headLinkPtr = NULL;

    // Get the List for the process-under-inspection.
    if (fd_ReadFromOffset(fd, listAddrOffset, &(iteratorPtr->threadObjList.List),
                             sizeof(iteratorPtr->threadObjList.List)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread obj list"));
    }

    // Get the ListChgCntRef for the process-under-inspection.
    if (fd_ReadFromOffset(fd, listChgCntAddrOffset, &(iteratorPtr->threadObjList.ListChgCntRef),
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
    InspType_t memberObjType, ///< [IN] The type of iterator to create.
    pid_t pid                         ///< [IN] The process to get the iterator for.
)
{
    // function prototype for the spy_GetListOfXXXChgCntRef family.
    typedef size_t** (*GetListChgCntRefFunc_t)(void);

    GetListChgCntRefFunc_t getListChgCntRefFunc;
    le_mem_PoolRef_t ThreadMemberObjIteratorPool;

    switch (memberObjType)
    {
        case INSPECT_INSP_TYPE_TIMER:
            getListChgCntRefFunc = spy_GetListOfTimersChgCntRef;
            ThreadMemberObjIteratorPool = TimerIteratorPool;
            break;

        case INSPECT_INSP_TYPE_MUTEX:
            getListChgCntRefFunc = spy_GetListOfMutexesChgCntRef;
            ThreadMemberObjIteratorPool = MutexIteratorPool;
            break;

        case INSPECT_INSP_TYPE_SEMAPHORE:
            getListChgCntRefFunc = spy_GetListOfSemaphoresChgCntRef;
            ThreadMemberObjIteratorPool = SemaphoreIteratorPool;
            break;

        default:
            INTERNAL_ERR("unexpected thread member object type %d.", memberObjType);
    }


    int fd = OpenProcMemFile(pid);

    // Get the address offset of the list of thread objs for the process to inspect.
    off_t threadObjListAddrOffset = GetRemoteAddress(pid, spy_GetListOfThreadObj());

    // Get the addr offset of the change counter of the list of thread objs for the process to
    // inspect.
    off_t threadObjListChgCntAddrOffset = GetRemoteAddress(pid, spy_GetListOfThreadObjsChgCntRef());

    // Get the address offset of the change counter of the list of thread member objs for the
    // process to inspect.
    off_t threadMemberObjListChgCntAddrOffset = GetRemoteAddress(pid, getListChgCntRefFunc());

    // Create the iterator.
    ThreadMemberObjIter_t* iteratorPtr = le_mem_ForceAlloc(ThreadMemberObjIteratorPool);
    iteratorPtr->procMemFd = fd;
    iteratorPtr->pid = pid;
    iteratorPtr->threadObjList.headLinkPtr = NULL;
    iteratorPtr->threadMemberObjList.headLinkPtr = NULL;
    // The list of thread member objs needs to be explicitly set NULL, in order to properly trigger
    // reading the first thread object (and hence the first thread member obj in it) in
    // GetNextThreadMemberObjLinkPtr.
    iteratorPtr->threadMemberObjList.List.headLinkPtr = NULL;

    // Get the list of thread objs for the process-under-inspection.
    if (fd_ReadFromOffset(fd, threadObjListAddrOffset, &(iteratorPtr->threadObjList.List),
                             sizeof(iteratorPtr->threadObjList.List)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread obj list"));
    }

    // Get the thread obj ListChgCntRef for the process-under-inspection.
    if (fd_ReadFromOffset(fd, threadObjListChgCntAddrOffset,
                          &(iteratorPtr->threadObjList.ListChgCntRef),
                          sizeof(iteratorPtr->threadObjList.ListChgCntRef)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread obj list change counter ref"));
    }

    // Get the thread member obj ListChgCntRef for the process-under-inspection.
    if (fd_ReadFromOffset(fd, threadMemberObjListChgCntAddrOffset,
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
 *      An iterator to the list of timers for the specified process.
 */
//--------------------------------------------------------------------------------------------------
static TimerIter_Ref_t CreateTimerIter
(
    pid_t pid ///< [IN] The process to get the iterator for.
)
{
    return (TimerIter_Ref_t)CreateThreadMemberObjIter(INSPECT_INSP_TYPE_TIMER, pid);
}

static MutexIter_Ref_t CreateMutexIter
(
    pid_t pid ///< [IN] The process to get the iterator for.
)
{
    return (MutexIter_Ref_t)CreateThreadMemberObjIter(INSPECT_INSP_TYPE_MUTEX, pid);
}

static SemaphoreIter_Ref_t CreateSemaphoreIter
(
    pid_t pid ///< [IN] The process to get the iterator for.
)
{
    return (SemaphoreIter_Ref_t)CreateThreadMemberObjIter(INSPECT_INSP_TYPE_SEMAPHORE, pid);
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
    if (fd_ReadFromOffset(iterator->procMemFd, (ssize_t)iterator->memPoolList.ListChgCntRef,
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
    if (fd_ReadFromOffset(iterator->procMemFd, (ssize_t)iterator->threadObjList.ListChgCntRef,
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
    if (fd_ReadFromOffset(iterator->procMemFd, (ssize_t)iterator->threadObjList.ListChgCntRef,
                          &threadObjListChgCnt, sizeof(threadObjListChgCnt)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread obj list change counter"));
    }

    if (fd_ReadFromOffset(iterator->procMemFd, (ssize_t)iterator->threadMemberObjList.ListChgCntRef,
                          &threadMemberObjListChgCnt, sizeof(threadMemberObjListChgCnt)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread member obj list change counter"));
    }

    return (threadObjListChgCnt + threadMemberObjListChgCnt);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next link of the provided link. This is for accessing a list in a remote process,
 * otherwise the doubly linked list API can simply be used. Note that "linkRef" is a ref to a
 * locally existing link obj, which is a link for a remote node. Therefore GetNextLink cannot be
 * called back-to-back.
 * After calling GetNextLink, the returned link ptr must be used to read the associated remote node
 * into the local memory space. One would then retrieve the link object from the node, and then
 * GetNextLink can be called on the ref of that link object.
 *
 * @return
 *      Pointer to a link of a node in the remote process
 */
//--------------------------------------------------------------------------------------------------
static le_dls_Link_t* GetNextLink
(
    RemoteListAccess_t* listInfoRef,    ///< [IN] Object for accessing a list in the remote process.
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
static MemPool_t* GetNextMemPool
(
    MemPoolIter_Ref_t iterator ///< [IN] The iterator to get the next mem pool from.
)
{
    le_dls_Link_t* linkPtr = GetNextLink(&(iterator->memPoolList),
                                         &(iterator->currMemPool.poolLink));

    if (linkPtr == NULL)
    {
        return NULL;
    }

    // Get the address of pool.
    MemPool_t* poolPtr = CONTAINER_OF(linkPtr, MemPool_t, poolLink);

    // Read the pool into our own memory.
    if (fd_ReadFromOffset(iterator->procMemFd, (ssize_t)poolPtr, &(iterator->currMemPool),
                          sizeof(iterator->currMemPool)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("mempool object"));
    }

    return &(iterator->currMemPool);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next thread object from the specified iterator. For other detail see GetNextMemPool.
 *
 * @return
 *      A thread object from the iterator's list of thread objects.
 */
//--------------------------------------------------------------------------------------------------
static ThreadObj_t* GetNextThreadObj
(
    ThreadObjIter_Ref_t iterator ///< [IN] The iterator to get the next thread obj from.
)
{
    le_dls_Link_t* linkPtr = GetNextLink(&(iterator->threadObjList),
                                         &(iterator->currThreadObj.link));

    if (linkPtr == NULL)
    {
        return NULL;
    }

    // Get the address of thread obj.
    ThreadObj_t* threadObjPtr = CONTAINER_OF(linkPtr, ThreadObj_t, link);

    // Read the thread obj into our own memory.
    if (fd_ReadFromOffset(iterator->procMemFd, (ssize_t)threadObjPtr, &(iterator->currThreadObj),
                          sizeof(iterator->currThreadObj)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("thread object"));
    }

    return &(iterator->currThreadObj);
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
    ThreadObj_t* threadObjRef         ///< [IN] Thread object ref.
)
{
    switch (memberObjType)
    {
        case INSPECT_INSP_TYPE_TIMER:
            return threadObjRef->timerRec.activeTimerList.headLinkPtr;
            break;

        case INSPECT_INSP_TYPE_MUTEX:
            return threadObjRef->mutexRec.lockedMutexList.headLinkPtr;
            break;

        default:
            INTERNAL_ERR("unexpected thread member object type %d.", memberObjType);
    }
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
    ThreadObj_t* localThreadObjRef;
    le_dls_Link_t* remThreadObjNextLinkPtr;
    le_dls_Link_t* remThreadMemberObjNextLinkPtr;

    // Get the next thread member obj
    remThreadMemberObjNextLinkPtr = GetNextLink(&(threadMemberObjItrRef->threadMemberObjList),
                                                currThreadMemberObjLinkPtr);
    while (remThreadMemberObjNextLinkPtr == NULL)
    {
        remThreadObjNextLinkPtr = GetNextLink(&(threadMemberObjItrRef->threadObjList),
                                              &(threadMemberObjItrRef->currThreadObj.link));

        // There are no more thread objects on the list (or list is empty)
        if (remThreadObjNextLinkPtr == NULL)
        {
            return NULL;
        }

        // Get the address of thread obj.
        ThreadObj_t* remThreadObjPtr = CONTAINER_OF(remThreadObjNextLinkPtr, ThreadObj_t, link);

        // Read the thread obj into our own memory, and update the local reference
        if (fd_ReadFromOffset(threadMemberObjItrRef->procMemFd, (ssize_t)remThreadObjPtr,
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
        remThreadMemberObjNextLinkPtr = GetNextLink(&(threadMemberObjItrRef->threadMemberObjList),
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
    TimerIter_Ref_t iterator ///< [IN] The iterator to get the next timer from.
)
{
    le_dls_Link_t* remThreadMemberObjNextLinkPtr =
        GetNextThreadMemberObjLinkPtr(INSPECT_INSP_TYPE_TIMER, (ThreadMemberObjIter_Ref_t)iterator);

    if (remThreadMemberObjNextLinkPtr == NULL)
    {
        return NULL;
    }

    // Get the address of timer.
    Timer_t* remTimerPtr = CONTAINER_OF(remThreadMemberObjNextLinkPtr, Timer_t, link);

    // Read the timer into our own memory.
    if (fd_ReadFromOffset(iterator->procMemFd, (ssize_t)remTimerPtr, &(iterator->currTimer),
                          sizeof(iterator->currTimer)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("timer object"));
    }

    return &(iterator->currTimer);
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
    MutexIter_Ref_t iterator ///< [IN] The iterator to get the next mutex from.
)
{
    le_dls_Link_t* remThreadMemberObjNextLinkPtr =
        GetNextThreadMemberObjLinkPtr(INSPECT_INSP_TYPE_MUTEX, (ThreadMemberObjIter_Ref_t)iterator);

    if (remThreadMemberObjNextLinkPtr == NULL)
    {
        return NULL;
    }

    // Get the address of mutex.
    Mutex_t* remMutexPtr = CONTAINER_OF(remThreadMemberObjNextLinkPtr, Mutex_t, lockedByThreadLink);

    // Read the mutex into our own memory.
    if (fd_ReadFromOffset(iterator->procMemFd, (ssize_t)remMutexPtr, &(iterator->currMutex),
                          sizeof(iterator->currMutex)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("mutex object"));
    }

    return &(iterator->currMutex);
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
    SemaphoreIter_Ref_t iterator ///< [IN] The iterator to get the next sempaphore from.
)
{
    Semaphore_t* remSemaphorePtr;
    ThreadObj_t* currThreadObjRef;

    // Create a local thread obj iterator based on the semaphore iterator that's passed in.
    ThreadObjIter_t threadObjIter;
    threadObjIter.pid = iterator->pid;
    threadObjIter.procMemFd = iterator->procMemFd;
    threadObjIter.threadObjList = iterator->threadObjList;
    threadObjIter.currThreadObj = iterator->currThreadObj;

    // Access the next thread obj directly since there's no "semaphore list" and each thread obj
    // owns at most one semaphore obj, in contrast of the logic of GetNextThreadMemberObjLinkPtr.
    do
    {
        // Get the next thread obj based on the semaphore iterator.
        currThreadObjRef = GetNextThreadObj(&threadObjIter);

        // Update the "current" thread object in the semaphore iterator.
        iterator->currThreadObj = threadObjIter.currThreadObj;
        // Also need to update the list (or rather the headLinkPtr in it). Otherwise next time
        // GetNextSemaphore is called, GetNextThreadObj still returns the "first" thread obj.
        iterator->threadObjList = threadObjIter.threadObjList;

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
    if (fd_ReadFromOffset(iterator->procMemFd, (ssize_t)remSemaphorePtr, &(iterator->currSemaphore),
                          sizeof(iterator->currSemaphore)) != LE_OK)
    {
        INTERNAL_ERR(REMOTE_READ_ERR("semaphore object"));
    }

    return &(iterator->currSemaphore);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a generic iterator according to the specified type.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteIter
(
    InspType_t inspectType, ///< [IN] The type of the iterator.
    void* iterator                  ///< [IN] The generic iterator to delete.
)
{
    INTERNAL_ERR_IF(iterator == NULL, "attempting to delete a NULL iterator.");

    switch (inspectType)
    {
        case INSPECT_INSP_TYPE_MEM_POOL:
            fd_Close(((MemPoolIter_Ref_t)iterator)->procMemFd);
            break;

        case INSPECT_INSP_TYPE_THREAD_OBJ:
            fd_Close(((ThreadObjIter_Ref_t)iterator)->procMemFd);
            break;

        case INSPECT_INSP_TYPE_TIMER:
            fd_Close(((TimerIter_Ref_t)iterator)->procMemFd);
            break;

        case INSPECT_INSP_TYPE_MUTEX:
            fd_Close(((MutexIter_Ref_t)iterator)->procMemFd);
            break;

        case INSPECT_INSP_TYPE_SEMAPHORE:
            fd_Close(((SemaphoreIter_Ref_t)iterator)->procMemFd);
            break;

        default:
            INTERNAL_ERR("Failed to delete iterator - unexpected iterator type %d.", inspectType);
    }

    le_mem_Release(iterator);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a mem pool iterator.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteMemPoolIter
(
    MemPoolIter_Ref_t iterator  ///< [IN] The iterator to delete.
)
{
    DeleteIter(INSPECT_INSP_TYPE_MEM_POOL, iterator);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a thread obj iterator.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteThreadObjIter
(
    ThreadObjIter_Ref_t iterator    ///< [IN] The iterator to delete.
)
{
    DeleteIter(INSPECT_INSP_TYPE_THREAD_OBJ, iterator);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a timer iterator.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteTimerIter
(
    TimerIter_Ref_t iterator    ///< [IN] The iterator to delete.
)
{
    DeleteIter(INSPECT_INSP_TYPE_TIMER, iterator);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a mutex iterator.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteMutexIter
(
    MutexIter_Ref_t iterator    ///< [IN] The iterator to delete.
)
{
    DeleteIter(INSPECT_INSP_TYPE_MUTEX, iterator);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a semaphore iterator.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteSemaphoreIter
(
    SemaphoreIter_Ref_t iterator    ///< [IN] The iterator to delete.
)
{
    DeleteIter(INSPECT_INSP_TYPE_SEMAPHORE, iterator);
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
        "    inspect [pools|threads|timers|mutexes|semaphores] [OPTIONS] PID\n"
        "\n"
        "DESCRIPTION:\n"
        "    inspect pools              Prints the memory pools usage for the specified process.\n"
        "    inspect threads            Prints the info of threads for the specified process.\n"
        "    inspect timers             Prints the info of timers in all threads for the specified process.\n"
        "    inspect mutexes            Prints the info of mutexes in all threads for the specified process.\n"
        "    inspect semaphores         Prints the info of semaphores in all threads for the specified process.\n"
        "\n"
        "OPTIONS:\n"
        "    -f\n"
        "        Periodically prints updated information for the process.\n"
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
    {"TOTAL BLKS",  "%*s",  NULL, "%*zu",       sizeof(size_t),              false, 0},
    {"USED BLKS",   "%*s",  NULL, "%*zu",       sizeof(size_t),              false, 0},
    {"MAX USED",    "%*s",  NULL, "%*zu",       sizeof(size_t),              false, 0},
    {"OVERFLOWS",   "%*s",  NULL, "%*zu",       sizeof(size_t),              false, 0},
    {"ALLOCS",      "%*s",  NULL, "%*"PRIu64"", sizeof(uint64_t),            false, 0},
    {"BLK BYTES",   "%*s",  NULL, "%*zu",       sizeof(size_t),              false, 0},
    {"USED BYTES",  "%*s",  NULL, "%*zu",       sizeof(size_t),              false, 0},
    {"MEMORY POOL", "%-*s", NULL, "%-*s",       LIMIT_MAX_MEM_POOL_NAME_LEN, true,  0},
    {"SUB-POOL",    "%*s",  NULL, "%*s",        0,                           true,  0}
};
static size_t MemPoolTableInfoSize = NUM_ARRAY_MEMBERS(MemPoolTableInfo);

static ColumnInfo_t ThreadObjTableInfo[] =
{
    {"NAME",             "%*s", NULL, "%*s",  MAX_THREAD_NAME_SIZE, true,  0},
    {"JOINABLE",         "%*s", NULL, "%*u",  sizeof(bool),         false, 0},
    {"STARTED",          "%*s", NULL, "%*u",  sizeof(bool),         false, 0},
    {"DETACHSTATE",      "%*s", NULL, "%*s",  0,                    true,  0},
    {"SCHED POLICY",     "%*s", NULL, "%*s",  0,                    true,  0},
    {"SCHED PARAM",      "%*s", NULL, "%*u",  sizeof(int),          false, 0},
    {"INHERIT SCHED",    "%*s", NULL, "%*s",  0,                    true,  0},
    {"CONTENTION SCOPE", "%*s", NULL, "%*s",  0,                    true,  0},
    {"GUARD SIZE",       "%*s", NULL, "%*zu", sizeof(size_t),       false, 0},
    {"STACK ADDR",       "%*s", NULL, "%*X",  sizeof(uint64_t),     false, 0},
    {"STACK SIZE",       "%*s", NULL, "%*zu", sizeof(size_t),       false, 0}
};
static size_t ThreadObjTableInfoSize = NUM_ARRAY_MEMBERS(ThreadObjTableInfo);

static ColumnInfo_t TimerTableInfo[] =
{
    {"NAME",         "%*s", NULL, "%*s",  LIMIT_MAX_TIMER_NAME_BYTES, true,  0},
    {"INTERVAL",     "%*s", NULL, "%*f",  sizeof(double),             false, 0},
    {"REPEAT COUNT", "%*s", NULL, "%*u",  sizeof(uint32_t),           false, 0},
    {"ISACTIVE",     "%*s", NULL, "%*u",  sizeof(bool),               false, 0},
    {"EXPIRY TIME",  "%*s", NULL, "%*f",  sizeof(double),             false, 0},
    {"EXPIRY COUNT", "%*s", NULL, "%*u",  sizeof(uint32_t),           false, 0}
};
static size_t TimerTableInfoSize = NUM_ARRAY_MEMBERS(TimerTableInfo);

static ColumnInfo_t MutexTableInfo[] =
{
    {"NAME",         "%*s", NULL, "%*s", MAX_NAME_BYTES,       true,  0},
    {"LOCK COUNT",   "%*s", NULL, "%*d", sizeof(int),          false, 0},
    {"RECURSIVE",    "%*s", NULL, "%*u", sizeof(bool),         false, 0},
    {"TRACEABLE",    "%*s", NULL, "%*u", sizeof(bool),         false, 0},
    {"WAITING LIST", "%*s", NULL, "%*s", MAX_THREAD_NAME_SIZE, true,  0}
};
static size_t MutexTableInfoSize = NUM_ARRAY_MEMBERS(MutexTableInfo);

static ColumnInfo_t SemaphoreTableInfo[] =
{
    {"NAME",         "%*s", NULL, "%*s", LIMIT_MAX_SEMAPHORE_NAME_BYTES, true,  0},
    {"TRACEABLE",    "%*s", NULL, "%*u", sizeof(bool),                   false, 0},
    {"WAITING LIST", "%*s", NULL, "%*s", MAX_THREAD_NAME_SIZE,           true,  0}
};
static size_t SemaphoreTableInfoSize = NUM_ARRAY_MEMBERS(SemaphoreTableInfo);


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
                                        NUM_ARRAY_MEMBERS(ThreadObjDetachStateTbl)));

        InitDisplayTableMaxDataSize("SCHED POLICY", table, tableSize,
                                    FindMaxStrSizeFromTable(ThreadObjSchedPolTbl,
                                        NUM_ARRAY_MEMBERS(ThreadObjSchedPolTbl)));

        InitDisplayTableMaxDataSize("INHERIT SCHED", table, tableSize,
                                    FindMaxStrSizeFromTable(ThreadObjInheritSchedTbl,
                                        NUM_ARRAY_MEMBERS(ThreadObjInheritSchedTbl)));

        InitDisplayTableMaxDataSize("CONTENTION SCOPE", table, tableSize,
                                    FindMaxStrSizeFromTable(ThreadObjContentionScopeTbl,
                                        NUM_ARRAY_MEMBERS(ThreadObjContentionScopeTbl)));
    }
    else if (table == MemPoolTableInfo)
    {
        size_t subPoolStrLen = strlen(SubPoolStr);
        size_t superPoolStrLen = strlen(SuperPoolStr);
        size_t subPoolColumnStrLen = subPoolStrLen > superPoolStrLen ? subPoolStrLen  :
                                                                       superPoolStrLen;
        InitDisplayTableMaxDataSize("SUB-POOL", table, tableSize, subPoolColumnStrLen);
    }

    int i;
    for (i = 0; i < tableSize; i++)
    {
        size_t headerTextWidth = strlen(table[i].colTitle);

        if (table[i].isString == false)
        {
            int maxDataWidth = (int)log10(exp2(table[i].maxDataSize*8))+1;
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
            InitDisplayTable(MemPoolTableInfo, NUM_ARRAY_MEMBERS(MemPoolTableInfo));
            break;

        case INSPECT_INSP_TYPE_THREAD_OBJ:
            InitDisplayTable(ThreadObjTableInfo, NUM_ARRAY_MEMBERS(ThreadObjTableInfo));
            break;

        case INSPECT_INSP_TYPE_TIMER:
            InitDisplayTable(TimerTableInfo, NUM_ARRAY_MEMBERS(TimerTableInfo));
            break;

        case INSPECT_INSP_TYPE_MUTEX:
            InitDisplayTable(MutexTableInfo, NUM_ARRAY_MEMBERS(MutexTableInfo));
            break;

        case INSPECT_INSP_TYPE_SEMAPHORE:
            InitDisplayTable(SemaphoreTableInfo, NUM_ARRAY_MEMBERS(SemaphoreTableInfo));
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
        index += snprintf((TableLineBuffer + index), (TableLineBytes - index),
                          table[i].titleFormat, table[i].colWidth, table[i].colTitle);
        index += snprintf((TableLineBuffer + index), (TableLineBytes - index), "%s", ColumnSpacers);
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
        index += snprintf((TableLineBuffer + index), (TableLineBytes - index), "%s",
                          table[i].colField);
        index += snprintf((TableLineBuffer + index), (TableLineBytes - index), "%s", ColumnSpacers);
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
    int* indexRef        ///< [OUT] Iterator to parse the table.
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
    #define inspectTypeStringSize 20
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
        for (i = 0; i < tableSize; i++)
        {
            printf("\"%s\"", table[i].colTitle);

            // For the last item, print the closing square bracket and comma for "Headers".
            // Otherwise print the comma separator.
            printf((i == (tableSize - 1)) ? "]," : ",");
        }

        // Print the data of "InspectType", "PID", and the beginning of "Data".
        printf("\"InspectType\":\"%s\",\"PID\":\"%d\",\"Data\":[", inspectTypeString, PidToInspect);
    }

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * The FillColField and ExportJsonData macros are meant to be used in the various PrintXXXInfo
 * functions. The macros process the data before they can be output.
 *
 * Since the "field" could be of various types, macros are used instead of X number of functions
 * each for a possible type.
 */
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * This macro fills the "colField" (Column field) of the supplied table. This prepares the table to
 * be printed in a human-readable format by PrintInfo.
 *
 * Note that the following needs to be prepared prior to using this macro:
 * - ColumnInfo_t* columnRef;
 * - int index = 0;
 */
//--------------------------------------------------------------------------------------------------
#define FillColField(field, table, tableSize) \
        columnRef = GetNextColumn(table, tableSize, &index); \
        snprintf(columnRef->colField, (columnRef->colWidth + 1), columnRef->fieldFormat, \
                 columnRef->colWidth, field);


//--------------------------------------------------------------------------------------------------
/**
 * This macro prints the inspected results ("field") in the JSON format.
 *
 * Note that the following needs to be prepared prior to using this macro:
 * - ColumnInfo_t* columnRef;
 * - int index = 0;
 * - bool isDataJsonArray = false;
 *
 * Sometimes there are multiple data in a "field". In that case, set isDataJsonArray to true and
 * replace the string pointed to by "field" with another string that has these multiple data
 * arranged in a JSON array. The thread names in the waiting lists of PrintMutexInfo and
 * PrintSemaphoreInfo are examples.
 */
//--------------------------------------------------------------------------------------------------
#define ExportJsonData(field, table, tableSize) \
        columnRef = GetNextColumn(table, tableSize, &index); \
        if (index == 1) printf("["); \
        if ((columnRef->isString) && (!isDataJsonArray)) printf("\""); \
        printf(columnRef->fieldFormat, 0, field); \
        if ((columnRef->isString) && (!isDataJsonArray)) printf("\""); \
        if (index == (tableSize)) printf("]"); else printf(",");


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

    // Needed for the macros
    ColumnInfo_t* columnRef;
    int index = 0;
    bool isDataJsonArray = false;

    // NOTE that the order has to correspond to the column orders in the corresponding table. Since
    // this order is "hardcoded" in a sense, one should avoid having multiple copies of these. The
    // same applies to other PrintXXXInfo functions.
    #define ProcessData \
        P(le_mem_GetObjectCount(memPool),       MemPoolTableInfo, MemPoolTableInfoSize); \
        P(poolStats.numBlocksInUse,             MemPoolTableInfo, MemPoolTableInfoSize); \
        P(poolStats.maxNumBlocksUsed,           MemPoolTableInfo, MemPoolTableInfoSize); \
        P(poolStats.numOverflows,               MemPoolTableInfo, MemPoolTableInfoSize); \
        P(poolStats.numAllocs,                  MemPoolTableInfo, MemPoolTableInfoSize); \
        P(blockSize,                            MemPoolTableInfo, MemPoolTableInfoSize); \
        P(blockSize*(poolStats.numBlocksInUse), MemPoolTableInfo, MemPoolTableInfoSize); \
        P(name,                                 MemPoolTableInfo, MemPoolTableInfoSize); \
        P(subPoolStr,                           MemPoolTableInfo, MemPoolTableInfoSize);

    if (!IsOutputJson)
    {
        #define P FillColField
        ProcessData
        #undef P

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

        #define P ExportJsonData
        ProcessData
        #undef P
    }

    #undef ProcessData

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print thread obj information to stdout.
 */
//--------------------------------------------------------------------------------------------------
static int PrintThreadObjInfo
(
    ThreadObj_t* threadObjRef   ///< [IN] ref to thread obj to be printed.
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


    ColumnInfo_t* columnRef;
    int index = 0;
    bool isDataJsonArray = false;

    #define ProcessData \
        P(threadObjRef->name,        ThreadObjTableInfo, ThreadObjTableInfoSize); \
        P(threadObjRef->isJoinable,  ThreadObjTableInfo, ThreadObjTableInfoSize); \
        P(threadObjRef->isStarted,   ThreadObjTableInfo, ThreadObjTableInfoSize); \
        P(detachStateStr,            ThreadObjTableInfo, ThreadObjTableInfoSize); \
        P(schedPolicyStr,            ThreadObjTableInfo, ThreadObjTableInfoSize); \
        P(schedParam.sched_priority, ThreadObjTableInfo, ThreadObjTableInfoSize); \
        P(inheritSchedStr,           ThreadObjTableInfo, ThreadObjTableInfoSize); \
        P(contentionScopeStr,        ThreadObjTableInfo, ThreadObjTableInfoSize); \
        P(guardSize,                 ThreadObjTableInfo, ThreadObjTableInfoSize); \
        P(stackAddr[0],              ThreadObjTableInfo, ThreadObjTableInfoSize); \
        P(stackSize,                 ThreadObjTableInfo, ThreadObjTableInfoSize);

    if (!IsOutputJson)
    {
        #define P FillColField
        ProcessData
        #undef P

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

        #define P ExportJsonData
        ProcessData
        #undef P
    }

    #undef ProcessData

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

    ColumnInfo_t* columnRef;
    int index = 0;
    bool isDataJsonArray = false;

    #define ProcessData \
        P(timerRef->name,        TimerTableInfo, TimerTableInfoSize); \
        P(interval,              TimerTableInfo, TimerTableInfoSize); \
        P(timerRef->repeatCount, TimerTableInfo, TimerTableInfoSize); \
        P(timerRef->isActive,    TimerTableInfo, TimerTableInfoSize); \
        P(expiryTime,            TimerTableInfo, TimerTableInfoSize); \
        P(timerRef->expiryCount, TimerTableInfo, TimerTableInfoSize);

    if (!IsOutputJson)
    {
        #define P FillColField
        ProcessData
        #undef P

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

        #define P ExportJsonData
        ProcessData
        #undef P
    }

    #undef ProcessData

    return lineCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Helper functions for GetWaitingListThreadNames
 */
//--------------------------------------------------------------------------------------------------
// Given a waiting list link ptr, get a ptr to the thread record
static mutex_ThreadRec_t* GetMutexThreadRecPtr
(
    le_dls_Link_t* currNodeLinkPtr  ///< [IN] waiting list link ptr.
)
{
    return CONTAINER_OF(currNodeLinkPtr, mutex_ThreadRec_t, waitingListLink);
}

// Given a thread rec ptr, get a ptr to the thread obj
static ThreadObj_t* GetThreadPtrFromMutexLink
(
    mutex_ThreadRec_t* currNodePtr  ///< [IN] thread record ptr.
)
{
    return CONTAINER_OF(currNodePtr, ThreadObj_t, mutexRec);
}

// Given a waiting list link ptr, get a ptr to the thread record
static sem_ThreadRec_t* GetSemThreadRecPtr
(
    le_dls_Link_t* currNodeLinkPtr  ///< [IN] waiting list link ptr.
)
{
    return CONTAINER_OF(currNodeLinkPtr, sem_ThreadRec_t, waitingListLink);
}

// Given a thread rec ptr, get a ptr to the thread obj
static ThreadObj_t* GetThreadPtrFromSemLink
(
    sem_ThreadRec_t* currNodePtr    ///< [IN] thread record ptr.
)
{
    return CONTAINER_OF(currNodePtr, ThreadObj_t, semaphoreRec);
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
    typedef void* (*GetThreadRecPtrFunc_t)(le_dls_Link_t* currNodeLinkPtr);
    typedef ThreadObj_t* (*GetThreadPtrFromLinkFunc_t)(void* currNodePtr);

    GetThreadRecPtrFunc_t getThreadRecPtrFunc;
    GetThreadPtrFromLinkFunc_t getThreadPtrFromLinkFunc;

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
            getThreadRecPtrFunc      = (GetThreadRecPtrFunc_t)     GetMutexThreadRecPtr;
            getThreadPtrFromLinkFunc = (GetThreadPtrFromLinkFunc_t)GetThreadPtrFromMutexLink;
            threadRecSize = sizeof(mutex_ThreadRec_t);
            break;

        case INSPECT_INSP_TYPE_SEMAPHORE:
            getThreadRecPtrFunc      = (GetThreadRecPtrFunc_t)     GetSemThreadRecPtr;
            getThreadPtrFromLinkFunc = (GetThreadPtrFromLinkFunc_t)GetThreadPtrFromSemLink;
            threadRecSize = sizeof(sem_ThreadRec_t);
            break;

        default:
            INTERNAL_ERR("Failed to get the waiting list link - unexpected inspect type %d.",
                         inspectType);
    }

    RemoteListAccess_t waitingList = {remoteWaitingList, NULL, NULL};
    le_dls_Link_t* currNodeLinkPtr = GetNextLink(&waitingList, NULL);

    void* currNodePtr;
    ThreadObj_t* currThreadPtr;

    ThreadRec_t localThreadRecCopy;
    ThreadObj_t localThreadObjCopy;

    int fd = OpenProcMemFile(PidToInspect);

    // Clear the thread name.
    memset(localThreadObjCopy.name, 0, sizeof(localThreadObjCopy.name));

    int i = 0;
    while (currNodeLinkPtr != NULL)
    {
        // From the thread record link ptr on the waiting list, get the associated thread obj ptr.
        currNodePtr = getThreadRecPtrFunc(currNodeLinkPtr);
        currThreadPtr = getThreadPtrFromLinkFunc(currNodePtr);

        // Read the thread obj into the local memory.
        if (fd_ReadFromOffset(fd, (ssize_t)currThreadPtr, &localThreadObjCopy,
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
        waitingThreadNames[i] = strndup(localThreadObjCopy.name, sizeof(localThreadObjCopy.name));
        i++;

        // Get the ptr to the the next node link on the waiting list, by reading the thread record
        // to the local memory first. GetNextLink must operate on a ref to a locally existing link.
        if (fd_ReadFromOffset(fd, (ssize_t)currNodePtr, &localThreadRecCopy, threadRecSize) !=
            LE_OK)
        {
            INTERNAL_ERR(REMOTE_READ_ERR("thread record with waiting list"));
        }

        le_dls_Link_t waitingListLink = GetWaitingListLink(inspectType, &localThreadRecCopy);

        currNodeLinkPtr = GetNextLink(&waitingList, &waitingListLink);
    }

    fd_Close(fd);
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

    ColumnInfo_t* columnRef;
    int index = 0;
    bool isDataJsonArray = false;

    #define ProcessData \
        P(mutexRef->name,        MutexTableInfo, MutexTableInfoSize); \
        P(mutexRef->lockCount,   MutexTableInfo, MutexTableInfoSize); \
        P(mutexRef->isRecursive, MutexTableInfo, MutexTableInfoSize); \
        P(mutexRef->isTraceable, MutexTableInfo, MutexTableInfoSize); \
        isDataJsonArray = true; \
        P(waitingThreadNames[0], MutexTableInfo, MutexTableInfoSize);

    if (!IsOutputJson)
    {
        #define P FillColField
        ProcessData
        #undef P

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

        #define P ExportJsonData
        #define waitingThreadNames &waitingThreadJsonArray
        ProcessData
        #undef waitingThreadNames
        #undef P
    }

    #undef ProcessData

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

    ColumnInfo_t* columnRef;
    int index = 0;
    bool isDataJsonArray = false;

    #define ProcessData \
        P(semaphoreRef->nameStr,     SemaphoreTableInfo, SemaphoreTableInfoSize); \
        P(semaphoreRef->isTraceable, SemaphoreTableInfo, SemaphoreTableInfoSize); \
        isDataJsonArray = true; \
        P(waitingThreadNames[0],     SemaphoreTableInfo, SemaphoreTableInfoSize);

    if (!IsOutputJson)
    {
        #define P FillColField
        ProcessData
        #undef P

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

        #define P ExportJsonData
        #define waitingThreadNames &waitingThreadJsonArray
        ProcessData
        #undef waitingThreadNames
        #undef P
    }

    #undef ProcessData

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
    InspType_t inspectType, ///< [IN] What to inspect.
    pid_t pid                       ///< [IN] For what process is the inspection carried out.
)
{
    // function prototype for the CreateXXXIter family.
    typedef void* (*CreateIterFunc_t)(pid_t pid);
    // function prototype for the GetXXXListChgCnt family.
    typedef size_t (*GetListChgCntFunc_t)(void* iterRef);
    // function prototype for the GetNextXXX family.
    typedef void* (*GetNextNodeFunc_t)(void* iterRef);
    // Function prototype for the DeleteXXXIter family.
    typedef void (*DeleteIterFunc_t)(void* iterRef);
    // Function prototype for the PrintXXXInfo family.
    typedef int (*PrintNodeInfoFunc_t)(void* nodeRef);

    CreateIterFunc_t createIterFunc;
    GetListChgCntFunc_t getListChgCntFunc;
    GetNextNodeFunc_t getNextNodeFunc;
    DeleteIterFunc_t deleteIterFunc;
    PrintNodeInfoFunc_t printNodeInfoFunc;

    // assigns the appropriate set of functions according to the inspection type.
    switch (inspectType)
    {
        case INSPECT_INSP_TYPE_MEM_POOL:
            createIterFunc          = (CreateIterFunc_t)         CreateMemPoolIter;
            getListChgCntFunc       = (GetListChgCntFunc_t)      GetMemPoolListChgCnt;
            getNextNodeFunc         = (GetNextNodeFunc_t)        GetNextMemPool;
            deleteIterFunc          = (DeleteIterFunc_t)         DeleteMemPoolIter;
            printNodeInfoFunc       = (PrintNodeInfoFunc_t)      PrintMemPoolInfo;
            break;

        case INSPECT_INSP_TYPE_THREAD_OBJ:
            createIterFunc          = (CreateIterFunc_t)         CreateThreadObjIter;
            getListChgCntFunc       = (GetListChgCntFunc_t)      GetThreadObjListChgCnt;
            getNextNodeFunc         = (GetNextNodeFunc_t)        GetNextThreadObj;
            deleteIterFunc          = (DeleteIterFunc_t)         DeleteThreadObjIter;
            printNodeInfoFunc       = (PrintNodeInfoFunc_t)      PrintThreadObjInfo;
            break;

        case INSPECT_INSP_TYPE_TIMER:
            createIterFunc          = (CreateIterFunc_t)         CreateTimerIter;
            getListChgCntFunc       = (GetListChgCntFunc_t)      GetThreadMemberObjListChgCnt;
            getNextNodeFunc         = (GetNextNodeFunc_t)        GetNextTimer;
            deleteIterFunc          = (DeleteIterFunc_t)         DeleteTimerIter;
            printNodeInfoFunc       = (PrintNodeInfoFunc_t)      PrintTimerInfo;
            break;

        case INSPECT_INSP_TYPE_MUTEX:
            createIterFunc          = (CreateIterFunc_t)         CreateMutexIter;
            getListChgCntFunc       = (GetListChgCntFunc_t)      GetThreadMemberObjListChgCnt;
            getNextNodeFunc         = (GetNextNodeFunc_t)        GetNextMutex;
            deleteIterFunc          = (DeleteIterFunc_t)         DeleteMutexIter;
            printNodeInfoFunc       = (PrintNodeInfoFunc_t)      PrintMutexInfo;
            break;

        case INSPECT_INSP_TYPE_SEMAPHORE:
            createIterFunc          = (CreateIterFunc_t)         CreateSemaphoreIter;
            getListChgCntFunc       = (GetListChgCntFunc_t)      GetThreadMemberObjListChgCnt;
            getNextNodeFunc         = (GetNextNodeFunc_t)        GetNextSemaphore;
            deleteIterFunc          = (DeleteIterFunc_t)         DeleteSemaphoreIter;
            printNodeInfoFunc       = (PrintNodeInfoFunc_t)      PrintSemaphoreInfo;
            break;

        default:
            INTERNAL_ERR("unexpected inspect type %d.", inspectType);
    }

    // Create an iterator.
    void* iterRef = createIterFunc(pid);

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

    deleteIterFunc(iterRef);
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
    // Perform the inspection.
    InspectFunc(InspectType, PidToInspect);
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
    else
    {
        fprintf(stderr, "Invalid command '%s'.\n", command);
        exit(EXIT_FAILURE);
    }
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
COMPONENT_INIT
{
    // Create a memory pool for iterators.
    MemPoolIteratorPool = le_mem_CreatePool("MemPooolIterators", sizeof(MemPoolIter_t));
    ThreadObjIteratorPool = le_mem_CreatePool("ThreadObjIterators", sizeof(ThreadObjIter_t));
    TimerIteratorPool = le_mem_CreatePool("TimerIterators", sizeof(TimerIter_t));
    MutexIteratorPool = le_mem_CreatePool("MutexIterators", sizeof(MutexIter_t));
    SemaphoreIteratorPool = le_mem_CreatePool("SemaphoreIterators", sizeof(SemaphoreIter_t));

    // The command-line has a command string followed by a PID.
    le_arg_AddPositionalCallback(CommandArgHandler);
    le_arg_AddPositionalCallback(PidArgHandler);

    // --help option causes everything else to be ignored, prints help, and exits.
    le_arg_SetFlagCallback(PrintHelp, NULL, "help");

    // -f option starts "following" (periodic updates until the program is terminated).
    le_arg_SetFlagVar(&IsFollowing, "f", NULL);

    // --interval=N option specifies the update period (implies -f).
    le_arg_SetIntCallback(FollowOptionCallback, NULL, "interval");

    // --format=json option outputs data to the specified file in JSON format.
    le_arg_SetStringCallback(FormatOptionCallback, NULL, "format");

    le_arg_Scan();

    InitDisplay(InspectType);

    // Start the inspection.
    InspectFunc(InspectType, PidToInspect);

    if (!IsFollowing)
    {
        exit(EXIT_SUCCESS);
    }
}
