//--------------------------------------------------------------------------------------------------
/** @file fdMonitor.c
 *
 * @section fdMonitor_DataStructures    Data Structures
 *
 *  - <b> FD Monitors </b> - One per monitored file descriptor.  Keeps track of the file descriptor,
 *                  what fd events are being monitored, and what thread is doing the monitoring.
 *
 * FD Monitor objects are allocated from the FD Monitor Pool and are kept on the FD Monitor List.
 *
 * @section fdMonitor_Algorithm     Algorithm
 *
 * When a file descriptor event is detected by the Event Loop, fdMon_Report() is called with
 * the FD Monitor Reference (a safe reference) and a bit map containing the events that were
 * detected.  fdMon_Report() queues a function call (DispatchToHandler()) to the calling thread.
 * When that function gets called, it does a look-up of the safe reference.  If it finds an
 * FD Monitor object matching that reference (it could have been deleted in the meantime), then
 * it calls its registered handler function for that event.
 *
 * The reason it was decided not to use Publish-Subscribe Events for this feature is that Event IDs
 * can't be deleted, and yet FD Monitors can.
 *
 * In some cases (e.g., with regular files), the fd doesn't support epoll().  In those cases, we
 * treat the fd as if it is always ready to be read from and written to.  If either EPOLLIN or
 * EPOLLOUT are enabled in the epoll events set for such an fd, DispatchToHandler() is immediately
 * queued to the thread's Event Queue
 *  - When the FD Monitor is created,
 *  - When DispatchToHandler() finishes running the handler function and the FD Monitor has not been
 *      deleted and still has at least one of EPOLLIN or EPOLLOUT enabled.
 *  - When le_fdMonitor_Enable() is called for an FD Monitor from outside that FD Monitor's handler.
 *
 * @section fdMonitor_Threads Threads
 *
 * Only the thread that creates an FD Monitor is allowed to perform operations on that FD Monitor,
 * including deleting the FD Monitor.
 *
 * The Safe Reference Map is shared between threads, though, so any access to it must be protected
 * from races.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "eventLoop.h"
#include "thread.h"
#include "fdMonitor.h"
#include "limit.h"

#include <pthread.h>


/// Maximum number of bytes in a File Descriptor Monitor's name, including the null terminator.
#define MAX_FD_MONITOR_NAME_BYTES  LIMIT_MAX_MEM_POOL_NAME_BYTES

/// The number of objects in the process-wide FD Monitor Pool, from which all FD Monitor
/// objects are allocated.
/// @todo Make this configurable.
#define DEFAULT_FD_MONITOR_POOL_SIZE 10


//--------------------------------------------------------------------------------------------------
/**
 * Thread-specific data key for the FD Monitor Ptr of the currently running fd event handler.
 *
 * This data item will be NULL if the thread is not currently running an fd event handler.
 **/
//--------------------------------------------------------------------------------------------------
static pthread_key_t FDMonitorPtrKey;


//--------------------------------------------------------------------------------------------------
/**
 * File Descriptor Monitor
 *
 * These keep track of file descriptors that are being monitored by a particular thread.
 * They are allocated from a per-thread FD Monitor Sub-Pool and are kept on the thread's
 * FD Monitor List.  In addition, each has a Safe Reference created from the
 * FD Monitor Reference Map.
 */
//--------------------------------------------------------------------------------------------------
typedef struct FdMonitor
{
    le_dls_Link_t           link;               ///< Used to link onto a thread's FD Monitor List.
    int                     fd;                 ///< File descriptor being monitored.
    uint32_t                epollEvents;        ///< epoll(7) flags for events being monitored.
    bool                    isAlwaysReady;      ///< Don't use epoll(7).  Treat as always ready.
    le_fdMonitor_Ref_t safeRef;            ///< Safe Reference for this object.
    event_PerThreadRec_t*   threadRecPtr;       ///< Ptr to per-thread data for monitoring thread.

    le_fdMonitor_HandlerFunc_t  handlerFunc;    ///< Handler function.
    void*                       contextPtr;     ///< The context pointer for this handler.

    char        name[MAX_FD_MONITOR_NAME_BYTES];            ///< UTF-8 name of this object.
}
FdMonitor_t;


//--------------------------------------------------------------------------------------------------
/**
 * FD Monitor Pool
 *
 * This is the main pool of FD Monitor objects from which FD Monitor objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t FdMonitorPool;


//--------------------------------------------------------------------------------------------------
/**
 * The Safe Reference Map to be used to create FD Monitor References.
 *
 * @warning This can be accessed by multiple threads.  Use the Mutex to protect it from races.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t FdMonitorRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * Mutex used to protect shared data structures in this module.
 */
//--------------------------------------------------------------------------------------------------
static pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;   // POSIX "Fast" mutex.

/// Locks the mutex.
#define LOCK    LE_ASSERT(pthread_mutex_lock(&Mutex) == 0);

/// Unlocks the mutex.
#define UNLOCK  LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0);


//--------------------------------------------------------------------------------------------------
/**
 * Trace reference used for controlling tracing in this module.
 **/
//--------------------------------------------------------------------------------------------------
static le_log_TraceRef_t TraceRef;

/// Macro used to generate trace output in this module.
/// Takes the same parameters as LE_DEBUG() et. al.
#define TRACE(...) LE_TRACE(TraceRef, ##__VA_ARGS__)

/// Macro used to check if trace output is enabled in this module.
#define IS_TRACE_ENABLED() LE_IS_TRACE_ENABLED(TraceRef)


// ==============================================
//  PRIVATE FUNCTIONS
// ==============================================

//--------------------------------------------------------------------------------------------------
/**
 * Converts a set of poll(2) event flags into a set of epoll(7) event flags.
 *
 * @return Bit map containing epoll(7) events flags.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t PollToEPoll
(
    short  pollFlags   ///< [in] Bit map containing poll(2) event flags.
)
//--------------------------------------------------------------------------------------------------
{
    uint32_t epollFlags = 0;

    if (pollFlags & POLLIN)
    {
        epollFlags |= EPOLLIN;
    }

    if (pollFlags & POLLPRI)
    {
        epollFlags |= EPOLLPRI;
    }

    if (pollFlags & POLLOUT)
    {
        epollFlags |= EPOLLOUT;
    }

    if (pollFlags & POLLHUP)
    {
        epollFlags |= EPOLLHUP;
    }

    if (pollFlags & POLLRDHUP)
    {
        epollFlags |= EPOLLRDHUP;
    }

    if (pollFlags & POLLERR)
    {
        epollFlags |= EPOLLERR;
    }

    return epollFlags;
}


//--------------------------------------------------------------------------------------------------
/**
 * Converts a set of epoll(7) event flags into a set of poll(2) event flags.
 *
 * @return Bit map containing poll(2) events flags.
 */
//--------------------------------------------------------------------------------------------------
static short EPollToPoll
(
    uint32_t  epollFlags    ///< [in] Bit map containing epoll(7) event flags.
)
//--------------------------------------------------------------------------------------------------
{
    short pollFlags = 0;

    if (epollFlags & EPOLLIN)
    {
        pollFlags |= POLLIN;
    }

    if (epollFlags & EPOLLPRI)
    {
        pollFlags |= POLLPRI;
    }

    if (epollFlags & EPOLLOUT)
    {
        pollFlags |= POLLOUT;
    }

    if (epollFlags & EPOLLHUP)
    {
        pollFlags |= POLLHUP;
    }

    if (epollFlags & EPOLLRDHUP)
    {
        pollFlags |= POLLRDHUP;
    }

    if (epollFlags & EPOLLERR)
    {
        pollFlags |= POLLERR;
    }

    return pollFlags;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a human readable string describing the fd events in a given bit map.
 *
 * @return buffPtr.
 */
//--------------------------------------------------------------------------------------------------
static const char* GetPollEventsText
(
    char* buffPtr,      ///< Pointer to buffer to put the string in.
    size_t buffSize,    ///< Size of the buffer, in bytes.
    short events        ///< Bit map of events (see 'man 2 poll').
)
//--------------------------------------------------------------------------------------------------
{
    int bytesWritten = snprintf(buffPtr, buffSize, "0x%hX ( ", events);

    if ((bytesWritten < buffSize) && (events & ~(POLLIN | POLLOUT | POLLHUP | POLLRDHUP | POLLERR)))
    {
        bytesWritten += snprintf(buffPtr + bytesWritten, buffSize - bytesWritten, "--invalid-- )");
    }
    else
    {
        if ((bytesWritten < buffSize) && (events & POLLIN))
        {
            bytesWritten += snprintf(buffPtr + bytesWritten, buffSize - bytesWritten, "POLLIN ");
        }
        if ((bytesWritten < buffSize) && (events & POLLOUT))
        {
            bytesWritten += snprintf(buffPtr + bytesWritten, buffSize - bytesWritten, "POLLOUT ");
        }
        if ((bytesWritten < buffSize) && (events & POLLHUP))
        {
            bytesWritten += snprintf(buffPtr + bytesWritten, buffSize - bytesWritten, "POLLHUP ");
        }
        if ((bytesWritten < buffSize) && (events & POLLRDHUP))
        {
            bytesWritten += snprintf(buffPtr + bytesWritten, buffSize - bytesWritten, "POLLRDHUP ");
        }
        if ((bytesWritten < buffSize) && (events & POLLERR))
        {
            bytesWritten += snprintf(buffPtr + bytesWritten, buffSize - bytesWritten, "POLLERR ");
        }
        if (bytesWritten < buffSize)
        {
            snprintf(buffPtr + bytesWritten, buffSize - bytesWritten, ")");
        }
    }

    return buffPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Tell epoll(7) to stop monitoring an FD Monitor object's fd.
 **/
//--------------------------------------------------------------------------------------------------
static void StopMonitoringFd
(
    FdMonitor_t* fdMonitorPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (fdMonitorPtr->isAlwaysReady)
    {
        return;
    }

    TRACE("Deleting fd %d (%s) from thread's epoll set.",
          fdMonitorPtr->fd,
          fdMonitorPtr->name);

    if (epoll_ctl(fdMonitorPtr->threadRecPtr->epollFd, EPOLL_CTL_DEL, fdMonitorPtr->fd, NULL) == -1)
    {
        if (errno == EBADF)
        {
            LE_CRIT("epoll_ctl(DEL) resulted in EBADF.  Probably because fd %d was closed"
                        " before deleting FD Monitor '%s'.",
                    fdMonitorPtr->fd,
                    fdMonitorPtr->name);
        }
        else if (errno == ENOENT)
        {
            LE_CRIT("epoll_ctl(DEL) resulted in ENOENT.  Probably because fd %d was closed"
                        " before deleting FD Monitor '%s'.",
                    fdMonitorPtr->fd,
                    fdMonitorPtr->name);
        }
        else
        {
            LE_FATAL("epoll_ctl(DEL) failed for fd %d. errno = %d (%m). FD Monitor '%s'.",
                     fdMonitorPtr->fd,
                     errno,
                     fdMonitorPtr->name);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a FD Monitor object for a given thread.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteFdMonitor
(
    FdMonitor_t*            fdMonitorPtr        ///< [in] Pointer to the FD Monitor to be deleted.
)
//--------------------------------------------------------------------------------------------------
{
    event_PerThreadRec_t*   perThreadRecPtr = thread_GetEventRecPtr();

    LE_ASSERT(perThreadRecPtr == fdMonitorPtr->threadRecPtr);

    // Remove the FD Monitor from the thread's FD Monitor List.
    le_dls_Remove(&perThreadRecPtr->fdMonitorList, &fdMonitorPtr->link);

    LOCK

    // Delete the Safe References used for the FD Monitor and any of its Handler objects.
    le_ref_DeleteRef(FdMonitorRefMap, fdMonitorPtr->safeRef);

    UNLOCK

    // Tell epoll(7) to stop monitoring this fd.
    StopMonitoringFd(fdMonitorPtr);

    // Release the object back to it's pool.
    le_mem_Release(fdMonitorPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Dispatch an FD Event to the appropriate registered handler function.
 */
//--------------------------------------------------------------------------------------------------
static void DispatchToHandler
(
    void* param1Ptr,    ///< FD Monitor safe reference.
    void* param2Ptr     ///< epoll() event flags.
)
//--------------------------------------------------------------------------------------------------
{
    uint32_t epollEventFlags = (uint32_t)(size_t)param2Ptr;

    LOCK

    // Get a pointer to the FD Monitor object for this fd.
    FdMonitor_t* fdMonitorPtr = le_ref_Lookup(FdMonitorRefMap, param1Ptr);

    UNLOCK

    // If the FD Monitor object has been deleted, we can just ignore this.
    if (fdMonitorPtr == NULL)
    {
        TRACE("Discarding events for non-existent FD Monitor %p.", param1Ptr);
        return;
    }

    // Sanity check: The FD monitor must belong to the current thread.
    LE_ASSERT(thread_GetEventRecPtr() == fdMonitorPtr->threadRecPtr);

    // Mask out any events that have been disabled since epoll_wait() reported these events to us.
    epollEventFlags &= (fdMonitorPtr->epollEvents | EPOLLERR | EPOLLHUP | EPOLLRDHUP);

    // If there's nothing left to report to the handler, don't call it.
    if (epollEventFlags == 0)
    {
        // Note: if the fd is always ready to read or write (is not supported by epoll()), then
        //       we will only end up in here if both POLLIN and POLLOUT are disabled, in which case
        //       returning now will prevent re-queuing of DispatchToHandler(), which is what we
        //       want.  When either POLLIN or POLLOUT are re-enabled, le_fdMonitor_Enable() will
        //       call fdMon_Report() to get things going again.
        return;
    }

    // Translate the epoll() events into poll() events.
    short pollEvents = EPollToPoll(epollEventFlags);

    if (IS_TRACE_ENABLED())
    {
        char eventsTextBuff[128];

        TRACE("Calling event handler for FD Monitor %s (fd %d, events %s).",
              fdMonitorPtr->name,
              fdMonitorPtr->fd,
              GetPollEventsText(eventsTextBuff, sizeof(eventsTextBuff), pollEvents));
    }

    // Increment the reference count on the Monitor object in case the handler deletes it.
    le_mem_AddRef(fdMonitorPtr);

    // Store a pointer to the FD Monitor as thread-specific data so le_fdMonitor_GetMonitor()
    // and le_fdMonitor_GetContextPtr() can find it.
    LE_ASSERT(pthread_setspecific(FDMonitorPtrKey, fdMonitorPtr) == 0);

    // Set the thread's event loop Context Pointer.
    event_SetCurrentContextPtr(fdMonitorPtr->contextPtr);

    // Call the handler function.
    fdMonitorPtr->handlerFunc(fdMonitorPtr->fd, pollEvents);

    // Clear the thread-specific pointer to the FD Monitor.
    LE_ASSERT(pthread_setspecific(FDMonitorPtrKey, NULL) == 0);

    // If this fd is always ready (is not supported by epoll) and either POLLIN or POLLOUT
    // are enabled, then queue up another dispatcher for this FD Monitor.
    // If neither are enabled, then le_fdMonitor_Enable() will queue the dispatcher
    // when one of them is re-enabled.
    if ((fdMonitorPtr->isAlwaysReady) && (fdMonitorPtr->epollEvents & (EPOLLIN | EPOLLOUT)))
    {
        fdMon_Report(fdMonitorPtr->safeRef, fdMonitorPtr->epollEvents & (EPOLLIN | EPOLLOUT));
    }

    // Release our reference.  We don't need the Monitor object anymore.
    le_mem_Release(fdMonitorPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Update the epoll(7) FD for a given FD Monitor object.
 **/
//--------------------------------------------------------------------------------------------------
static void UpdateEpollFd
(
    FdMonitor_t*    monitorPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (monitorPtr->isAlwaysReady)
    {
        return;
    }

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = monitorPtr->epollEvents;
    ev.data.ptr = monitorPtr->safeRef;

    int epollFd = monitorPtr->threadRecPtr->epollFd;

    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, monitorPtr->fd, &ev) == -1)
    {
        if (errno == EBADF)
        {
            LE_FATAL("epoll_ctl(MOD) resulted in EBADF.  Probably because fd %d was closed"
                     " before deleting FD Monitor '%s'.",
                  monitorPtr->fd,
                  monitorPtr->name);
        }
        else
        {
            LE_FATAL("epoll_ctl(MOD) failed for fd %d and events %x on monitor '%s'. Errno %d (%m)",
                    monitorPtr->fd,
                    monitorPtr->epollEvents,
                    monitorPtr->name,
                    errno);
        }
    }
}



// ==============================================
//  INTER-MODULE FUNCTIONS
// ==============================================

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the FD Monitor module.
 *
 * This function must be called exactly once at process start-up, before any other FD Monitor
 * functions are called.
 */
//--------------------------------------------------------------------------------------------------
void fdMon_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Create the FD Monitor Pool from which FD Monitor objects are to be allocated.
    /// @todo Make this configurable.
    FdMonitorPool = le_mem_CreatePool("FdMonitor", sizeof(FdMonitor_t));
    le_mem_ExpandPool(FdMonitorPool, DEFAULT_FD_MONITOR_POOL_SIZE);

    // Create the Safe Reference Map.
    /// @todo Make this configurable.
    FdMonitorRefMap = le_ref_CreateMap("FdMonitors", DEFAULT_FD_MONITOR_POOL_SIZE);

    // Get a reference to the trace keyword that is used to control tracing in this module.
    TraceRef = le_log_GetTraceRef("fdMonitor");

    // Create the thread-specific data key for the FD Monitor Ptr of the current running handler.
    LE_ASSERT(pthread_key_create(&FDMonitorPtrKey, NULL) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the FD Monitor part of the Event Loop API's per-thread record.
 *
 * This function must be called exactly once at thread start-up, before any other FD Monitor
 * functions are called by that thread.
 */
//--------------------------------------------------------------------------------------------------
void fdMon_InitThread
(
    event_PerThreadRec_t* perThreadRecPtr
)
//--------------------------------------------------------------------------------------------------
{
    perThreadRecPtr->fdMonitorList = LE_DLS_LIST_INIT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Report FD Events.
 *
 * This is called by the Event Loop when it detects events on a file descriptor that is being
 * monitored.
 */
//--------------------------------------------------------------------------------------------------
void fdMon_Report
(
    void*       safeRef,        ///< [in] Safe Reference for the FD Monitor object for the fd.
    uint32_t    eventFlags      ///< [in] OR'd together event flags from epoll_wait().
)
//--------------------------------------------------------------------------------------------------
{
    le_event_QueueFunction(DispatchToHandler, safeRef, (void*)(ssize_t)eventFlags);
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete all FD Monitor objects for the calling thread.
 *
 * @note    This is only called by the thread that is being destructed.
 */
//--------------------------------------------------------------------------------------------------
void fdMon_DestructThread
(
    event_PerThreadRec_t* perThreadRecPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr;

    while ((linkPtr = le_dls_Peek(&perThreadRecPtr->fdMonitorList)) != NULL)
    {
        FdMonitor_t* fdMonitorPtr = CONTAINER_OF(linkPtr, FdMonitor_t, link);
        DeleteFdMonitor(fdMonitorPtr);
    }
}


// ==============================================
//  PUBLIC API FUNCTIONS
// ==============================================

//--------------------------------------------------------------------------------------------------
/**
 * Creates a File Descriptor Monitor.
 *
 * Creates an object that will monitor a given file descriptor for events.
 *
 * The monitoring will be performed by the event loop of the thread that created the Monitor object.
 * If that thread is blocked, no events will be detected for that file descriptor until that
 * thread is unblocked and returns to its event loop.
 *
 * Events that can be enabled for monitoring:
 *
 * - @c POLLIN = Data available to read.
 * - @c POLLPRI = Urgent data available to read (e.g., out-of-band data on a socket).
 * - @c POLLOUT = Writing to the fd should accept some data now.
 *
 * These are bitmask values and can be combined using the bit-wise OR operator ('|').
 *
 * The following events are always monitored, even if not requested:
 *
 * - @c POLLRDHUP = Other end of stream socket closed or shutdown.
 * - @c POLLERR = Error occurred.
 * - @c POLLHUP = Hang up.
 *
 * @return
 *      Reference to the object, which is needed for later deletion.
 *
 * @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_fdMonitor_Ref_t le_fdMonitor_Create
(
    const char*             name,       ///< [in] Name of the object (for diagnostics).
    int                     fd,         ///< [in] File descriptor to be monitored for events.
    le_fdMonitor_HandlerFunc_t handlerFunc, ///< [in] Handler function.
    short                   events      ///< [in] Initial set of events to be monitored.
)
//--------------------------------------------------------------------------------------------------
{
    // Get a pointer to the thread-specific event loop data record.
    event_PerThreadRec_t* perThreadRecPtr = thread_GetEventRecPtr();

    // Allocate the object.
    FdMonitor_t* fdMonitorPtr = le_mem_ForceAlloc(FdMonitorPool);

    // Initialize the object.
    fdMonitorPtr->link = LE_DLS_LINK_INIT;
    fdMonitorPtr->fd = fd;
    fdMonitorPtr->epollEvents = PollToEPoll(events) | EPOLLWAKEUP;  // Non-deferrable by default.
    fdMonitorPtr->isAlwaysReady = false;
    fdMonitorPtr->threadRecPtr = perThreadRecPtr;
    fdMonitorPtr->handlerFunc = handlerFunc;
    fdMonitorPtr->contextPtr = NULL;

    // Copy the name into it.
    if (le_utf8_Copy(fdMonitorPtr->name, name, sizeof(fdMonitorPtr->name), NULL) == LE_OVERFLOW)
    {
        LE_WARN("FD Monitor object name '%s' truncated to '%s'.", name, fdMonitorPtr->name);
    }

    LOCK

    // Create a safe reference for the object.
    fdMonitorPtr->safeRef = le_ref_CreateRef(FdMonitorRefMap, fdMonitorPtr);

    // Add it to the thread's FD Monitor list.
    le_dls_Queue(&perThreadRecPtr->fdMonitorList, &fdMonitorPtr->link);

    // Tell epoll(7) to start monitoring this fd.
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = fdMonitorPtr->epollEvents;
    ev.data.ptr = fdMonitorPtr->safeRef;
    if (epoll_ctl(perThreadRecPtr->epollFd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        if (errno == EPERM)
        {
            LE_DEBUG("fd %d doesn't support epoll(), assuming always readable and writeable.", fd);
            fdMonitorPtr->isAlwaysReady = true;

            // If either EPOLLIN or EPOLLOUT are enabled, queue up the handler for this now.
            uint32_t epollEvents = fdMonitorPtr->epollEvents & (EPOLLIN | EPOLLOUT);
            if (epollEvents != 0)
            {
                fdMon_Report(fdMonitorPtr->safeRef, epollEvents);
            }
        }
        else
        {
            LE_FATAL("epoll_ctl(ADD) failed for fd %d. errno = %d (%m)", fd, errno);
        }
    }

    UNLOCK

    return fdMonitorPtr->safeRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Enables monitoring for events on a file descriptor.
 *
 * Events that can be enabled for monitoring:
 *
 * - @c POLLIN = Data available to read.
 * - @c POLLPRI = Urgent data available to read (e.g., out-of-band data on a socket).
 * - @c POLLOUT = Writing to the fd should accept some data now.
 *
 * These are bitmask values and can be combined using the bit-wise OR operator ('|').
 */
//--------------------------------------------------------------------------------------------------
void le_fdMonitor_Enable
(
    le_fdMonitor_Ref_t  monitorRef, ///< [in] Reference to the File Descriptor Monitor object.
    short               events      ///< [in] Bit map of events.
)
//--------------------------------------------------------------------------------------------------
{
    // Look up the File Descriptor Monitor object using the safe reference provided.
    // Note that the safe reference map is shared by all threads in the process, so it
    // must be protected using the mutex.  The File Descriptor Monitor objects, on the other
    // hand, are only allowed to be accessed by the one thread that created them, so it is
    // safe to unlock the mutex after doing the safe reference lookup.
    LOCK
    FdMonitor_t* monitorPtr = le_ref_Lookup(FdMonitorRefMap, monitorRef);
    UNLOCK

    LE_FATAL_IF(monitorPtr == NULL, "File Descriptor Monitor %p doesn't exist!", monitorRef);
    LE_FATAL_IF(thread_GetEventRecPtr() != monitorPtr->threadRecPtr,
                "FD Monitor '%s' (fd %d) is owned by another thread.",
                monitorPtr->name,
                monitorPtr->fd);

    short filteredEvents = events & (POLLIN | POLLOUT | POLLPRI);

    if (filteredEvents != events)
    {
        char textBuff[64];

        LE_WARN("Attempt to enable events that can't be disabled (%s).",
                GetPollEventsText(textBuff, sizeof(textBuff), events & ~filteredEvents));
    }

    uint32_t epollEvents = PollToEPoll(filteredEvents);

    // If the fd doesn't support epoll, we assume it is always ready for read and write.
    // As long as EPOLLIN or EPOLLOUT (or both) is enabled for one of these fds, DispatchToHandler()
    // keeps re-queueing itself to the thread's event queue.  But it will stop doing that if
    // EPOLLIN and EPOLLOUT are both disabled.  So, here is where we get things going again when
    // EPOLLIN or EPOLLOUT is enabled outside DispatchToHandler() for that fd.
    if ( (monitorPtr->isAlwaysReady)
        && (epollEvents & (EPOLLIN | EPOLLOUT))
        && ((monitorPtr->epollEvents & (EPOLLIN | EPOLLOUT)) == 0) )
    {
        // Fetch the pointer to the FD Monitor from thread-specific data.
        // This will be NULL if we are not inside an FD Monitor handler.
        FdMonitor_t* handlerMonitorPtr = pthread_getspecific(FDMonitorPtrKey);

        // If no handler is running or some other fd's handler is running,
        if ((handlerMonitorPtr == NULL) || (handlerMonitorPtr->safeRef == monitorRef))
        {
            // Queue up DispatchToHandler() for this fd.
            fdMon_Report(monitorRef, epollEvents & (EPOLLIN | EPOLLOUT));
        }
    }

    // Bit-wise OR the newly enabled event flags into the FD Monitor's epoll(7) flags set.
    monitorPtr->epollEvents |= epollEvents;

    UpdateEpollFd(monitorPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Disables monitoring for events on a file descriptor.
 *
 * Events that can be disabled for monitoring:
 *
 * - @c POLLIN = Data available to read.
 * - @c POLLPRI = Urgent data available to read (e.g., out-of-band data on a socket).
 * - @c POLLOUT = Writing to the fd should accept some data now.
 *
 * These are bitmask values and can be combined using the bit-wise OR operator ('|').
 */
//--------------------------------------------------------------------------------------------------
void le_fdMonitor_Disable
(
    le_fdMonitor_Ref_t  monitorRef, ///< [in] Reference to the File Descriptor Monitor object.
    short               events      ///< [in] Bit map of events.
)
//--------------------------------------------------------------------------------------------------
{
    // Look up the File Descriptor Monitor object using the safe reference provided.
    // Note that the safe reference map is shared by all threads in the process, so it
    // must be protected using the mutex.  The File Descriptor Monitor objects, on the other
    // hand, are only allowed to be accessed by the one thread that created them, so it is
    // safe to unlock the mutex after doing the safe reference lookup.
    LOCK
    FdMonitor_t* monitorPtr = le_ref_Lookup(FdMonitorRefMap, monitorRef);
    UNLOCK

    LE_FATAL_IF(monitorPtr == NULL, "File Descriptor Monitor %p doesn't exist!", monitorRef);
    LE_FATAL_IF(thread_GetEventRecPtr() != monitorPtr->threadRecPtr,
                "FD Monitor '%s' (fd %d) is owned by another thread.",
                monitorPtr->name,
                monitorPtr->fd);

    short filteredEvents = events & (POLLIN | POLLOUT | POLLPRI);

    LE_WARN_IF(filteredEvents != events,
               "Only POLLIN, POLLOUT, and POLLPRI events can be disabled. (fd monitor '%s')",
               monitorPtr->name);

    // Convert the events from POLLxx events to EPOLLxx events.
    uint32_t epollEvents = PollToEPoll(filteredEvents);

    // Remove them from the FD Monitor's epoll(7) flags set.
    monitorPtr->epollEvents &= (~epollEvents);

    UpdateEpollFd(monitorPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets if processing of events on a given fd is deferrable (the system is allowed to go to
 * sleep while there are monitored events pending for this fd) or urgent (the system will be kept
 * awake until there are no monitored events waiting to be handled for this fd).
 *
 * If the process has @c CAP_EPOLLWAKEUP (or @c CAP_BLOCK_SUSPEND) capability, then fd events are
 * considered urgent by default.
 *
 * If the process does not have @c CAP_EPOLLWAKEUP (or @c CAP_BLOCK_SUSPEND) capability, then fd
 * events are always deferrable, and calls to this function have no effect.
 */
//--------------------------------------------------------------------------------------------------
void le_fdMonitor_SetDeferrable
(
    le_fdMonitor_Ref_t monitorRef,  ///< [in] Reference to the File Descriptor Monitor object.
    bool               isDeferrable ///< [in] true (deferrable) or false (urgent).
)
//--------------------------------------------------------------------------------------------------
{
    // Look up the File Descriptor Monitor object using the safe reference provided.
    // Note that the safe reference map is shared by all threads in the process, so it
    // must be protected using the mutex.  The File Descriptor Monitor objects, on the other
    // hand, are only allowed to be accessed by the one thread that created them, so it is
    // safe to unlock the mutex after doing the safe reference lookup.
    LOCK
    FdMonitor_t* monitorPtr = le_ref_Lookup(FdMonitorRefMap, monitorRef);
    UNLOCK

    LE_FATAL_IF(monitorPtr == NULL, "File Descriptor Monitor %p doesn't exist!", monitorRef);
    LE_FATAL_IF(thread_GetEventRecPtr() != monitorPtr->threadRecPtr,
                "FD Monitor '%s' (fd %d) is owned by another thread.",
                monitorPtr->name,
                monitorPtr->fd);

    // Set/clear the EPOLLWAKEUP flag in the FD Monitor's epoll(7) flags set.
    if (isDeferrable)
    {
        monitorPtr->epollEvents &= ~EPOLLWAKEUP;
    }
    else
    {
        monitorPtr->epollEvents |= EPOLLWAKEUP;
    }

    UpdateEpollFd(monitorPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the Context Pointer for File Descriptor Monitor's handler function.  This can be retrieved
 * by the handler by calling le_fdMonitor_GetContextPtr() when the handler function is running.
 */
//--------------------------------------------------------------------------------------------------
void le_fdMonitor_SetContextPtr
(
    le_fdMonitor_Ref_t  monitorRef, ///< [in] Reference to the File Descriptor Monitor.
    void*               contextPtr  ///< [in] Opaque context pointer value.
)
//--------------------------------------------------------------------------------------------------
{
    // Look up the File Descriptor Monitor object using the safe reference provided.
    // Note that the safe reference map is shared by all threads in the process, so it
    // must be protected using the mutex.  The File Descriptor Monitor objects, on the other
    // hand, are only allowed to be accessed by the one thread that created them, so it is
    // safe to unlock the mutex after doing the safe reference lookup.
    LOCK
    FdMonitor_t* monitorPtr = le_ref_Lookup(FdMonitorRefMap, monitorRef);
    UNLOCK

    LE_FATAL_IF(monitorPtr == NULL, "File Descriptor Monitor %p doesn't exist!", monitorRef);
    LE_FATAL_IF(thread_GetEventRecPtr() != monitorPtr->threadRecPtr,
                "FD Monitor '%s' (fd %d) is owned by another thread.",
                monitorPtr->name,
                monitorPtr->fd);

    monitorPtr->contextPtr = contextPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the Context Pointer for File Descriptor Monitor's handler function.
 *
 * @return  The context pointer set using le_fdMonitor_SetContextPtr(), or NULL if it hasn't been
 *          set.
 *
 * @note    This only works inside the handler function.
 */
//--------------------------------------------------------------------------------------------------
void* le_fdMonitor_GetContextPtr
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Fetch the pointer to the FD Monitor from thread-specific data.
    FdMonitor_t* monitorPtr = pthread_getspecific(FDMonitorPtrKey);

    LE_FATAL_IF(monitorPtr == NULL, "Not inside an fd event handler.");

    return monitorPtr->contextPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to the File Descriptor Monitor whose handler function is currently running.
 *
 * @return  File Descriptor Monitor reference.
 *
 * @note    This only works inside the handler function.
 **/
//--------------------------------------------------------------------------------------------------
le_fdMonitor_Ref_t le_fdMonitor_GetMonitor
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Fetch the pointer to the FD Monitor from thread-specific data.
    FdMonitor_t* monitorPtr = pthread_getspecific(FDMonitorPtrKey);

    LE_FATAL_IF(monitorPtr == NULL, "Not inside an fd event handler.");

    return monitorPtr->safeRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the file descriptor that an FD Monitor object is monitoring.
 *
 * @return  The fd.
 */
//--------------------------------------------------------------------------------------------------
int le_fdMonitor_GetFd
(
    le_fdMonitor_Ref_t  monitorRef  ///< [in] Reference to the File Descriptor Monitor.
)
//--------------------------------------------------------------------------------------------------
{
    LOCK
    FdMonitor_t* monitorPtr = le_ref_Lookup(FdMonitorRefMap, monitorRef);
    UNLOCK

    LE_FATAL_IF(monitorPtr == NULL, "File Descriptor Monitor %p doesn't exist!", monitorRef);

    return monitorPtr->fd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a file descriptor monitor object.
 */
//--------------------------------------------------------------------------------------------------
void le_fdMonitor_Delete
(
    le_fdMonitor_Ref_t monitorRef  ///< [in] Reference to the File Descriptor Monitor object.
)
//--------------------------------------------------------------------------------------------------
{
    LOCK
    FdMonitor_t* monitorPtr = le_ref_Lookup(FdMonitorRefMap, monitorRef);
    UNLOCK

    LE_FATAL_IF(monitorPtr == NULL, "FD Monitor object %p does not exist!", monitorRef);

    LE_FATAL_IF(monitorPtr->threadRecPtr != thread_GetEventRecPtr(),
                "FD Monitor '%s' (%d) is owned by another thread.",
                monitorPtr->name,
                monitorPtr->fd);

    DeleteFdMonitor(monitorPtr);
}
