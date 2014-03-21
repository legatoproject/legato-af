//--------------------------------------------------------------------------------------------------
/** @file fdMonitor.c
 *
 * @section fdMon_DataStructures    Data Structures
 *
 *  - <b> FD Monitors </b> - One per monitored file descriptor.  Keeps track of the file descriptor,
 *                  what fd events are being monitored, and what thread is doing the monitoring.
 *
 * FD Monitor objects are allocated from the FD Monitor Pool and are kept on the FD Monitor List.
 *
 * @section fdMon_Algorithm     Algorithm
 *
 * When a file descriptor event is detected by the Event Loop, fdMon_Report() is called with
 * the FD Monitor Reference (a safe reference) and the type of event that was detected.
 * fdMon_Report() queues a function call (DispatchToHandler) to the calling thread. When that
 * function gets called, it does a look-up of the safe reference.  If it finds an FD Monitor
 * object matching that reference, then it calls its registered handler function for that event.
 *
 * The reason it was decided not to use Publish-Subscribe Events for this feature is that Event IDs
 * can't be deleted, and yet FD Monitors can.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "eventLoop.h"
#include "thread.h"
#include "fdMonitor.h"

#include <pthread.h>
#include <sys/epoll.h>


/// Maximum number of bytes in a File Descriptor Monitor's name, including the null terminator.
#define MAX_FD_MONITOR_NAME_BYTES  32

/// The number of objects in the process-wide FD Monitor Pool, from which all FD Monitor
/// objects are allocated.
/// @todo Make this configurable.
#define DEFAULT_FD_MONITOR_POOL_SIZE 10


/// Pointer to a File Descriptor Monitor object.
typedef struct FdMonitor* FdMonitorPtr_t;


//--------------------------------------------------------------------------------------------------
/**
 * Handler object.
 *
 * This stores the registration information for a handler function.  They are allocated from the
 * Handler Pool and are stored on an FD Monitor object's Handler List.  Outside this module,
 * these are referred to using a safe reference.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_event_FdHandlerFunc_t    handlerFunc;    ///< The function.
    void*                       contextPtr;     ///< The context pointer for this handler.
    FdMonitorPtr_t              monitorPtr;     ///< Pointer to the FD Monitor for this handler.
    void*                       safeRef;        ///< Safe Reference for this object.
}
Handler_t;


//--------------------------------------------------------------------------------------------------
/**
 * File Descriptor Monitor
 *
 * These keep track of file descriptors that are being monitored by a particular thread.
 * They are allocated from a per-thread FD Monitor Sub-Pool and are kept on the thread's
 * FD Monitor List.  In addition, each has a Safe Reference created from the
 * FD Monitor Reference Map.
 *
 * @warning These can be accessed by multiple threads.
 *          Great care must be taken to prevent races when accessing these objects.
 */
//--------------------------------------------------------------------------------------------------
typedef struct FdMonitor
{
    le_dls_Link_t           link;               ///< Used to link onto a thread's FD Monitor List.
    int                     fd;                 ///< File descriptor being monitored.
    uint32_t                epollEvents;        ///< epoll(7) flags for events being monitored.
    le_event_FdMonitorRef_t safeRef;            ///< Safe Reference for this object.
    event_PerThreadRec_t*   threadRecPtr;       ///< Ptr to per-thread data for monitoring thread.

    Handler_t   handlerArray[LE_EVENT_NUM_FD_EVENT_TYPES];  ///< Handler objects (1 per event type)
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
 * The Safe Reference Map to be used to create FD Event Handler References.
 *
 * @warning This can be accessed by multiple threads.  Use the Mutex to protect it from races.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t HandlerRefMap;


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



// ==============================================
//  PRIVATE FUNCTIONS
// ==============================================

//--------------------------------------------------------------------------------------------------
/**
 * Converts an Event Loop API file descriptor event type identifier into an epoll(7) event flag.
 *
 * See 'man epoll_ctl' for more information.
 *
 * @return A single epoll event flag.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t ConvertToEPollFlag
(
    le_event_FdEventType_t  fdEventType ///< [in] The Event Loop API's fd event type identifier.
)
//--------------------------------------------------------------------------------------------------
{
    switch (fdEventType)
    {
        case LE_EVENT_FD_READABLE:

            return EPOLLIN;

        case LE_EVENT_FD_READABLE_URGENT:

            return EPOLLPRI;

        case LE_EVENT_FD_WRITEABLE:

            return EPOLLOUT;

        case LE_EVENT_FD_WRITE_HANG_UP:

            return EPOLLHUP;

        case LE_EVENT_FD_READ_HANG_UP:

            return EPOLLRDHUP;

        case LE_EVENT_FD_ERROR:

            return EPOLLERR;
    }

    LE_FATAL("Invalid fd event type %d.", fdEventType);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a pointer to a constant string containing a human readable name for a type of fd event.
 *
 * @return Pointer to the name.
 */
//--------------------------------------------------------------------------------------------------
const char* GetFdEventTypeName
(
    le_event_FdEventType_t eventType
)
//--------------------------------------------------------------------------------------------------
{
    switch (eventType)
    {
        case LE_EVENT_FD_READABLE:

            return "readable";

        case LE_EVENT_FD_READABLE_URGENT:

            return "readable-urgent";

        case LE_EVENT_FD_WRITEABLE:

            return "writeable";

        case LE_EVENT_FD_WRITE_HANG_UP:

            return "write-hangup";

        case LE_EVENT_FD_READ_HANG_UP:

            return "read-hangup";

        case LE_EVENT_FD_ERROR:

            return "error";
    }

    LE_FATAL("Unknown event type %d.", eventType);
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
    TRACE("Deleting fd %d (%s) from thread's epoll set.",
          fdMonitorPtr->fd,
          fdMonitorPtr->name);

    if (epoll_ctl(fdMonitorPtr->threadRecPtr->epollFd, EPOLL_CTL_DEL, fdMonitorPtr->fd, NULL) == -1)
    {
        if (errno == EBADF)
        {
            TRACE("epoll_ctl(DEL) for fd %d resulted in EBADF.  Probably because connection"
                  " closed before deleting FD Monitor %s.",
                  fdMonitorPtr->fd,
                  fdMonitorPtr->name);
        }
        else if (errno == ENOENT)
        {
            TRACE("epoll_ctl(DEL) for fd %d resulted in ENOENT.  Probably because we stopped"
                  " monitoring before deleting the FD Monitor %s.",
                  fdMonitorPtr->fd,
                  fdMonitorPtr->name);
        }
        else
        {
            LE_FATAL("epoll_ctl(DEL) failed for fd %d. errno = %d (%m)",
                     fdMonitorPtr->fd,
                     errno);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a FD Monitor object for a given thread.
 *
 * @warning Assumes that the Mutex lock is already held.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteFdMonitor
(
    FdMonitor_t*            fdMonitorPtr        ///< [in] Pointer to the FD Monitor to be deleted.
)
//--------------------------------------------------------------------------------------------------
{
    int i;
    event_PerThreadRec_t*   perThreadRecPtr = thread_GetEventRecPtr();

    LE_ASSERT(perThreadRecPtr == fdMonitorPtr->threadRecPtr);

    // Remove the FD Monitor from the thread's FD Monitor List.
    le_dls_Remove(&perThreadRecPtr->fdMonitorList, &fdMonitorPtr->link);

    LOCK

    // Delete the Safe References used for the FD Monitor and any of its Handler objects.
    le_ref_DeleteRef(FdMonitorRefMap, fdMonitorPtr->safeRef);
    for (i = 0; i < LE_EVENT_NUM_FD_EVENT_TYPES; i++)
    {
        void* safeRef = fdMonitorPtr->handlerArray[i].safeRef;
        if (safeRef != NULL)
        {
            le_ref_DeleteRef(HandlerRefMap, safeRef);
        }
    }

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
    void* param2Ptr     ///< FD event type.
)
//--------------------------------------------------------------------------------------------------
{
    le_event_FdEventType_t eventType = (le_event_FdEventType_t)param2Ptr;
    event_PerThreadRec_t* perThreadRecPtr = thread_GetEventRecPtr();

    LOCK

    // Get a pointer to the FD Monitor object for this fd.
    FdMonitor_t* fdMonitorPtr = le_ref_Lookup(FdMonitorRefMap, param1Ptr);

    UNLOCK

    // If the FD Monitor object has been deleted, we can just ignore this.
    if (fdMonitorPtr != NULL)
    {
        LE_ASSERT(perThreadRecPtr == fdMonitorPtr->threadRecPtr);

        Handler_t* handlerPtr = &(fdMonitorPtr->handlerArray[eventType]);

        if (handlerPtr->handlerFunc != NULL)
        {
            // Set the thread's Context Pointer.
            event_SetCurrentContextPtr(handlerPtr->contextPtr);

            // Call the handler function.
            handlerPtr->handlerFunc(fdMonitorPtr->fd);
        }
        else
        {
            TRACE("Discarding event %s for FD Monitor %s (fd %d).",
                  GetFdEventTypeName(eventType),
                  fdMonitorPtr->name,
                  fdMonitorPtr->fd);

            // If this is a write hang-up, then we need to tell epoll to stop monitoring
            // this fd, because otherwise we could end up wasting power and spamming the
            // log with debug messages while we detect and discard this event over and over.
            if (eventType == LE_EVENT_FD_WRITE_HANG_UP)
            {
                StopMonitoringFd(fdMonitorPtr);
            }
        }
    }
    else
    {
        TRACE("Discarding event %s for non-existent FD Monitor.", GetFdEventTypeName(eventType));
    }
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
    struct epoll_event ev;

    ev.events = monitorPtr->epollEvents;
    ev.data.ptr = monitorPtr->safeRef;

    int epollFd = monitorPtr->threadRecPtr->epollFd;

    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, monitorPtr->fd, &ev) == -1)
    {
        if (errno == EBADF)
        {
            TRACE("epoll_ctl(MOD) for fd %d resulted in EBADF.  Probably because connection"
                  " closed before deleting FD Monitor %s.",
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


//--------------------------------------------------------------------------------------------------
/**
 * Enables monitoring of a specific event on a specific FD.
 **/
//--------------------------------------------------------------------------------------------------
static void EnableFdMonitoring
(
    FdMonitor_t*            monitorPtr,
    le_event_FdEventType_t  eventType
)
//--------------------------------------------------------------------------------------------------
{
    // Add the epoll event flag to the flag set being monitored for this fd.
    // (Not necessary for EPOLLERR or EPOLLHUP.  They are always monitored, no matter what.)
    uint32_t epollEventFlag = ConvertToEPollFlag(eventType); // Note: this range checks eventType.
    if ((epollEventFlag != EPOLLERR) && (epollEventFlag != EPOLLHUP))
    {
        monitorPtr->epollEvents |= epollEventFlag;

        UpdateEpollFd(monitorPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Disables monitoring of a specific event on a specific FD.
 **/
//--------------------------------------------------------------------------------------------------
static void DisableFdMonitoring
(
    FdMonitor_t*            monitorPtr,
    le_event_FdEventType_t  eventType
)
//--------------------------------------------------------------------------------------------------
{
    // Remove the epoll event flag from the flag set being monitored for this fd.
    // (Not possible for EPOLLERR or EPOLLHUP.  They are always monitored, no matter what.)
    uint32_t epollEventFlag = ConvertToEPollFlag(eventType); // Note: this range checks eventType.
    if ((epollEventFlag != EPOLLERR) && (epollEventFlag != EPOLLHUP))
    {
        monitorPtr->epollEvents &= (~epollEventFlag);

        UpdateEpollFd(monitorPtr);
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

    // Create the Safe Reference Maps.
    /// @todo Make this configurable.
    FdMonitorRefMap = le_ref_CreateMap("FdMonitors", DEFAULT_FD_MONITOR_POOL_SIZE);
    HandlerRefMap = le_ref_CreateMap("FdEventHandlers",
                                     DEFAULT_FD_MONITOR_POOL_SIZE * LE_EVENT_NUM_FD_EVENT_TYPES);

    // Get a reference to the trace keyword that is used to control tracing in this module.
    TraceRef = le_log_GetTraceRef("fdMonitor");
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
    // Queue up function calls for any flags set in epoll's event.

    if (eventFlags & EPOLLIN)
    {
        le_event_QueueFunction(DispatchToHandler, safeRef, (void*)LE_EVENT_FD_READABLE);
    }

    if (eventFlags & EPOLLPRI)
    {
        le_event_QueueFunction(DispatchToHandler, safeRef, (void*)LE_EVENT_FD_READABLE_URGENT);
    }

    if (eventFlags & EPOLLOUT)
    {
        le_event_QueueFunction(DispatchToHandler, safeRef, (void*)LE_EVENT_FD_WRITEABLE);
    }

    if (eventFlags & EPOLLHUP)
    {
        le_event_QueueFunction(DispatchToHandler, safeRef, (void*)LE_EVENT_FD_WRITE_HANG_UP);
    }

    if (eventFlags & EPOLLRDHUP)
    {
        le_event_QueueFunction(DispatchToHandler, safeRef, (void*)LE_EVENT_FD_READ_HANG_UP);
    }

    if (eventFlags & EPOLLERR)
    {
        le_event_QueueFunction(DispatchToHandler, safeRef, (void*)LE_EVENT_FD_ERROR);
    }

    LE_CRIT_IF(eventFlags & ~(EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLHUP | EPOLLRDHUP | EPOLLERR),
               "Extra flags found in fd event report. (%xd)",
               eventFlags);
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete all FD Monitor objects for the calling thread.
 */
//--------------------------------------------------------------------------------------------------
void fdMon_DestructThread
(
    event_PerThreadRec_t* perThreadRecPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&perThreadRecPtr->fdMonitorList);
    while (linkPtr != NULL)
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
 * @return
 *      A reference to the object, which is needed for later deletion.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_FdMonitorRef_t le_event_CreateFdMonitor
(
    const char*     name,       ///< [in] Name of the object (for diagnostics).
    int             fd          ///< [in] File descriptor to be monitored for events.
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
    fdMonitorPtr->threadRecPtr = perThreadRecPtr;
    memset(fdMonitorPtr->handlerArray, 0, sizeof(fdMonitorPtr->handlerArray));

    // To start with, no events are in the set to be monitored.  They will be added as handlers
    // are registered for them. (Although, EPOLLHUP and EPOLLERR will always be monitored
    // regardless of what flags we specify).  We use epoll in "level-triggered mode".
    fdMonitorPtr->epollEvents = 0;

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
    ev.events = fdMonitorPtr->epollEvents;
    ev.data.ptr = fdMonitorPtr->safeRef;
    if (epoll_ctl(perThreadRecPtr->epollFd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        LE_FATAL("epoll_ctl(ADD) failed for fd %d. errno = %d (%m)", fd, errno);
    }

    UNLOCK

    return fdMonitorPtr->safeRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Registers a handler for a specific type of file descriptor event with a given
 * File Descriptor Monitor object.
 *
 * When the handler function is called, it will be called by the the thread that registered
 * the handler, which must also be the same thread that created the FD Monitor object.
 *
 * @return A reference to the handler function.
 *
 * @note    Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_FdHandlerRef_t le_event_SetFdHandler
(
    le_event_FdMonitorRef_t  monitorRef, ///< [in] Reference to the File Descriptor Monitor object.
    le_event_FdEventType_t   eventType,  ///< [in] The type of event to be reported to this handler.
    le_event_FdHandlerFunc_t handlerFunc ///< [in] The handler function.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(handlerFunc != NULL);

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

    // Get a pointer to the Handler object in the appropriate spot for this type of event in the
    // FD Monitor's array of handlers.
    Handler_t* handlerPtr = &(monitorPtr->handlerArray[eventType]);

    // Double check that no one has tried setting this handler yet.
    LE_FATAL_IF(handlerPtr->handlerFunc != NULL,
                "FD handler already set for event '%s' on FD Monitor '%s' (fd %d).",
                GetFdEventTypeName(eventType),
                monitorPtr->name,
                monitorPtr->fd);

    // Initialize the Handler object.
    handlerPtr->handlerFunc = handlerFunc;
    handlerPtr->contextPtr = NULL;
    handlerPtr->monitorPtr = monitorPtr;
    LOCK
    handlerPtr->safeRef = le_ref_CreateRef(HandlerRefMap, handlerPtr);
    UNLOCK

    // Enable the monitoring of this event.
    EnableFdMonitoring(monitorPtr, eventType);

    return handlerPtr->safeRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the Context Pointer for a handler for a file descriptor event.  This can be retrieved
 * by the handler by calling le_event_GetContextPtr() when the handler function is running.
 */
//--------------------------------------------------------------------------------------------------
void le_event_SetFdHandlerContextPtr
(
    le_event_FdHandlerRef_t handlerRef, ///< [in] Reference to the handler.
    void*                   contextPtr  ///< [in] Opaque context pointer value.
)
//--------------------------------------------------------------------------------------------------
{
    LOCK

    Handler_t* handlerPtr = le_ref_Lookup(HandlerRefMap, handlerRef);

    LE_ASSERT(handlerPtr != NULL);

    handlerPtr->contextPtr = contextPtr;

    UNLOCK
}


//--------------------------------------------------------------------------------------------------
/**
 * Deregisters a handler for a file descriptor event.
 */
//--------------------------------------------------------------------------------------------------
void le_event_ClearFdHandler
(
    le_event_FdHandlerRef_t  handlerRef  ///< [in] Reference to the handler.
)
//--------------------------------------------------------------------------------------------------
{
    // Look up the Handler object using the safe reference provided.
    // Note that the safe reference map is shared by all threads in the process, so it
    // must be protected using the mutex.  The Handler objects, on the other
    // hand, are only allowed to be accessed by the one thread that created them, so it is
    // safe to unlock the mutex after doing the safe reference lookup.
    LOCK
    Handler_t* handlerPtr = le_ref_Lookup(HandlerRefMap, handlerRef);
    UNLOCK

    LE_FATAL_IF(handlerPtr == NULL, "FD event handler %p doesn't exist!", handlerRef);

    FdMonitor_t* monitorPtr = handlerPtr->monitorPtr;

    LE_FATAL_IF(thread_GetEventRecPtr() != monitorPtr->threadRecPtr,
                "FD Monitor '%s' (fd %d) is owned by another thread.",
                monitorPtr->name,
                monitorPtr->fd);

    LE_ASSERT(handlerPtr->handlerFunc != NULL);

    le_event_FdEventType_t eventType = INDEX_OF_ARRAY_MEMBER(monitorPtr->handlerArray, handlerPtr);
    LE_ASSERT(eventType < LE_EVENT_NUM_FD_EVENT_TYPES);

    // Clear the Handler object.
    handlerPtr->handlerFunc = NULL;
    handlerPtr->contextPtr = NULL;
    LOCK
    le_ref_DeleteRef(HandlerRefMap, handlerPtr->safeRef);
    UNLOCK

    // Disable the monitoring of this event.
    DisableFdMonitoring(monitorPtr, eventType);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a file descriptor monitor object.
 *
 * This will automatically remove all handlers added to the object.
 */
//--------------------------------------------------------------------------------------------------
void le_event_DeleteFdMonitor
(
    le_event_FdMonitorRef_t monitorRef  ///< [in] Reference to the File Descriptor Monitor object.
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
