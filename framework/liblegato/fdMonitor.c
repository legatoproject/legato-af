//--------------------------------------------------------------------------------------------------
/** @file fdMonitor.c
 *
 * Shared code for file descriptor monitoring.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"

#include "fdMonitor.h"
#include "thread.h"

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
 * The static safe reference map used to create FD Monitor References.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(FdMonitors, LE_CONFIG_MAX_FD_MONITOR_POOL_SIZE);

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

// ==============================================
//  PRIVATE FUNCTIONS
// ==============================================

//--------------------------------------------------------------------------------------------------
/**
 * Deletes an FD Monitor object for a given thread.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteFdMonitor
(
    fdMon_t *fdMonitorPtr ///< [in] Pointer to the FD Monitor to be deleted.
)
{
    event_PerThreadRec_t *perThreadRecPtr = thread_GetEventRecPtr();

    LE_ASSERT(perThreadRecPtr == fdMonitorPtr->threadRecPtr);

    // Disable all events so all pending events are dropped.
    fdMonitorPtr->eventFlags = 0;

    // Remove the FD Monitor from the thread's FD Monitor List.
    le_dls_Remove(&perThreadRecPtr->fdMonitorList, &fdMonitorPtr->link);

    LOCK

    // Delete the Safe References used for the FD Monitor and any of its Handler objects.
    le_ref_DeleteRef(FdMonitorRefMap, fdMonitorPtr->safeRef);

    UNLOCK

    fa_fdMon_Delete(fdMonitorPtr);
    le_mem_Release(fdMonitorPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Dispatch an FD Event to the appropriate registered handler function.
 */
//--------------------------------------------------------------------------------------------------
static void DispatchToHandler
(
    void        *fdMonRef,  ///< FD Monitor safe reference.
    void        *param      ///< Event flags.
)
{
    fdMon_t     *fdMonitorPtr;
    uint32_t     flags = (uintptr_t) param;

    LOCK

    // Get a pointer to the FD Monitor object for this fd.
    fdMonitorPtr = le_ref_Lookup(FdMonitorRefMap, fdMonRef);

    UNLOCK

    // If the FD Monitor object has been deleted, we can just ignore this.
    if (fdMonitorPtr == NULL)
    {
        LE_DEBUG("Discarding events for non-existent FD Monitor %p.", fdMonRef);
        return;
    }

    // Sanity check: The FD monitor must belong to the current thread.
    LE_ASSERT(thread_GetEventRecPtr() == fdMonitorPtr->threadRecPtr);

    // Increment the reference count on the Monitor object in case the handler deletes it.
    le_mem_AddRef(fdMonitorPtr);

    // Store a pointer to the FD Monitor as thread-specific data so le_fdMonitor_GetMonitor()
    // and le_fdMonitor_GetContextPtr() can find it.
    LE_ASSERT(pthread_setspecific(FDMonitorPtrKey, fdMonitorPtr) == 0);

    // Set the thread's event loop Context Pointer.
    event_SetCurrentContextPtr(fdMonitorPtr->contextPtr);

    fa_fdMon_DispatchToHandler(fdMonitorPtr, flags);

    // Clear the thread-specific pointer to the FD Monitor.
    LE_ASSERT(pthread_setspecific(FDMonitorPtrKey, NULL) == 0);

     // Release our reference.  We don't need the Monitor object anymore.
    le_mem_Release(fdMonitorPtr);
}

// ==============================================
//  INTER-MODULE FUNCTIONS
// ==============================================

//--------------------------------------------------------------------------------------------------
/**
 * Get a human readable string describing the fd events in a given bit map.
 *
 * @return buffPtr.
 */
//--------------------------------------------------------------------------------------------------
const char *fdMon_GetEventsText
(
    char    *buffPtr,   ///< [out] Pointer to buffer to put the string in.
    size_t   buffSize,  ///< [in]  Size of the buffer, in bytes.
    short    events     ///< [in]  Bit map of events (see 'man 2 poll').
)
{
    size_t bytesWritten = snprintf(buffPtr, buffSize, "0x%hX ( ", events);

    if ((bytesWritten < buffSize) && (events & POLLIN))
    {
        bytesWritten += snprintf(buffPtr + bytesWritten, buffSize - bytesWritten, "POLLIN ");
    }
    if ((bytesWritten < buffSize) && (events & POLLOUT))
    {
        bytesWritten += snprintf(buffPtr + bytesWritten, buffSize - bytesWritten, "POLLOUT ");
    }
    if ((bytesWritten < buffSize) && (events & POLLPRI))
    {
        bytesWritten += snprintf(buffPtr + bytesWritten, buffSize - bytesWritten, "POLLPRI ");
    }
    if ((bytesWritten < buffSize) && (events & POLLHUP))
    {
        bytesWritten += snprintf(buffPtr + bytesWritten, buffSize - bytesWritten, "POLLHUP ");
    }
    if ((bytesWritten < buffSize) && (events & POLLNVAL))
    {
        bytesWritten += snprintf(buffPtr + bytesWritten, buffSize - bytesWritten, "POLLNVAL ");
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

    return buffPtr;
}

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
{
    // Create the Safe Reference Map.
    FdMonitorRefMap = le_ref_InitStaticMap(FdMonitors, LE_CONFIG_MAX_FD_MONITOR_POOL_SIZE);

    FdMonitorPool = fa_fdMon_Init();

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
    event_PerThreadRec_t *perThreadRecPtr
)
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
    void        *safeRef,       ///< [in] Safe Reference for the FD Monitor object for the fd.
    uint32_t     eventFlags     ///< [in] OR'd together event flags.
)
{
    le_event_QueueFunction(&DispatchToHandler, safeRef, (void *) (uintptr_t) eventFlags);
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
    event_PerThreadRec_t *perThreadRecPtr
)
{
    fdMon_t         *fdMonitorPtr;
    le_dls_Link_t   *linkPtr;

    while ((linkPtr = le_dls_Peek(&perThreadRecPtr->fdMonitorList)) != NULL)
    {
        fdMonitorPtr = CONTAINER_OF(linkPtr, fdMon_t, link);
        DeleteFdMonitor(fdMonitorPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Signal a file descriptor has pending events.
 *
 * @note: This should only be used on customized file descriptors.  Other file descriptors are
 *        checked by the event loop automatically.
 **/
//--------------------------------------------------------------------------------------------------
void fdMon_SignalFd
(
    int         fd,         ///< [in] Reference to the File Descriptor Monitor object
    uint32_t    eventFlags  ///< [in] Event flags to signal
)
{
    fdMon_t             *fdMonitorPtr;
    int                  oldState;
    le_ref_IterRef_t     iter = le_ref_GetIterator(FdMonitorRefMap);

    while (le_ref_NextNode(iter) == LE_OK)
    {
        fdMonitorPtr = le_ref_GetValue(iter);
        if ((fdMonitorPtr->fd == fd)                &&
            (fdMonitorPtr->eventFlags & eventFlags))
        {
            oldState = event_Lock();
            fa_event_TriggerEvent_NoLock(fdMonitorPtr->threadRecPtr);
            event_Unlock(oldState);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Lock the fdMonitor mutex.
 */
//--------------------------------------------------------------------------------------------------
void fdMon_Lock
(
    void
)
{
    LE_ASSERT(pthread_mutex_lock(&Mutex) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Unlock the fdMonitor mutex.
 */
//--------------------------------------------------------------------------------------------------
void fdMon_Unlock
(
    void
)
{
    LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0);
}

// ==============================================
//  PUBLIC API FUNCTIONS
// ==============================================

//--------------------------------------------------------------------------------------------------
// Create definitions for inlineable functions
//
// See le_fdMonitor.h for bodies & documentation
//--------------------------------------------------------------------------------------------------
#if !LE_CONFIG_FD_MONITOR_NAMES_ENABLED
LE_DEFINE_INLINE le_fdMonitor_Ref_t le_fdMonitor_Create
(
    const char                  *name,
    int                          fd,
    le_fdMonitor_HandlerFunc_t   handlerFunc,
    short                        events
);
#endif

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
#if LE_CONFIG_FD_MONITOR_NAMES_ENABLED
le_fdMonitor_Ref_t le_fdMonitor_Create
(
    const char*                 name,           ///< [in] Name of the object (for diagnostics).
    int                         fd,             ///< [in] File descriptor to be monitored for
                                                ///<      events.
    le_fdMonitor_HandlerFunc_t  handlerFunc,    ///< [in] Handler function.
    short                       events          ///< [in] Initial set of events to be monitored.
)
#else /* !LE_CONFIG_FD_MONITOR_NAMES_ENABLED */
le_fdMonitor_Ref_t _le_fdMonitor_Create
(
    int                         fd,             ///< [in] File descriptor to be monitored for
                                                ///<      events.
    le_fdMonitor_HandlerFunc_t  handlerFunc,    ///< [in] Handler function.
    short                       events          ///< [in] Initial set of events to be monitored.
)
#endif /* end !LE_CONFIG_FD_MONITOR_NAMES_ENABLED */
{
    event_PerThreadRec_t *recPtr = thread_GetEventRecPtr();

    // Allocate the object.
    fdMon_t *fdMonitorPtr = le_mem_ForceAlloc(FdMonitorPool);

    // Initialize the object.
    fdMonitorPtr->link = LE_DLS_LINK_INIT;
    fdMonitorPtr->fd = fd;
    fdMonitorPtr->threadRecPtr = recPtr;
    fdMonitorPtr->handlerFunc = handlerFunc;
    fdMonitorPtr->contextPtr = NULL;
    fdMonitorPtr->eventFlags = events;

#if LE_CONFIG_FD_MONITOR_NAMES_ENABLED
    // Copy the name into it.
    if (le_utf8_Copy(fdMonitorPtr->name, name, sizeof(fdMonitorPtr->name), NULL) == LE_OVERFLOW)
    {
        LE_WARN("FD Monitor object name '%s' truncated to '%s'.", name, fdMonitorPtr->name);
    }
#endif /* end LE_CONFIG_FD_MONITOR_NAMES_ENABLED */

    LOCK

    // Create a safe reference for the object.
    fdMonitorPtr->safeRef = le_ref_CreateRef(FdMonitorRefMap, fdMonitorPtr);

    // Add it to the thread's FD Monitor list.
    le_dls_Queue(&recPtr->fdMonitorList, &fdMonitorPtr->link);

    // Perform platform-specific setup.
    fa_fdMon_Create(fdMonitorPtr);

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
{
    char     textBuff[64];
    fdMon_t *handlerMonitorPtr;
    fdMon_t *monitorPtr;
    short    filteredEvents;

    // Look up the File Descriptor Monitor object using the safe reference provided.
    // Note that the safe reference map is shared by all threads in the process, so it
    // must be protected using the mutex.  The File Descriptor Monitor objects, on the other
    // hand, are only allowed to be accessed by the one thread that created them, so it is
    // safe to unlock the mutex after doing the safe reference lookup.
    LOCK
    monitorPtr = le_ref_Lookup(FdMonitorRefMap, monitorRef);
    UNLOCK

    LE_FATAL_IF(monitorPtr == NULL, "File Descriptor Monitor %p doesn't exist!", monitorRef);
    LE_FATAL_IF(thread_GetEventRecPtr() != monitorPtr->threadRecPtr,
                "FD Monitor '%s' (fd %d) is owned by another thread.",
                FDMON_NAME(monitorPtr->name),
                monitorPtr->fd);

    handlerMonitorPtr = pthread_getspecific(FDMonitorPtrKey);
    filteredEvents = fa_fdMon_Enable(monitorPtr, handlerMonitorPtr, events);

    LE_WARN_IF(filteredEvents != events, "Attempt to enable events that are always enabled (%s).",
        fdMon_GetEventsText(textBuff, sizeof(textBuff), events & ~filteredEvents));

    // Bit-wise OR the newly enabled event flags into the FD Monitor's event group flags set.
    monitorPtr->eventFlags |= filteredEvents;
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
{
    char     textBuff[64];
    fdMon_t *handlerMonitorPtr;
    fdMon_t *monitorPtr;
    short    filteredEvents;

    // Look up the File Descriptor Monitor object using the safe reference provided.
    // Note that the safe reference map is shared by all threads in the process, so it
    // must be protected using the mutex.  The File Descriptor Monitor objects, on the other
    // hand, are only allowed to be accessed by the one thread that created them, so it is
    // safe to unlock the mutex after doing the safe reference lookup.
    LOCK
    monitorPtr = le_ref_Lookup(FdMonitorRefMap, monitorRef);
    UNLOCK

    LE_FATAL_IF(monitorPtr == NULL, "File Descriptor Monitor %p doesn't exist!", monitorRef);
    LE_FATAL_IF(thread_GetEventRecPtr() != monitorPtr->threadRecPtr,
                "FD Monitor '%s' (fd %d) is owned by another thread.",
                FDMON_NAME(monitorPtr->name),
                monitorPtr->fd);

    handlerMonitorPtr = pthread_getspecific(FDMonitorPtrKey);
    filteredEvents = fa_fdMon_Disable(monitorPtr, handlerMonitorPtr, events);

    LE_WARN_IF(filteredEvents != events, "Attempt to disable events that can't be disabled (%s).",
        fdMon_GetEventsText(textBuff, sizeof(textBuff), events & ~filteredEvents));

    // Remove them from the FD Monitor's event group flags set.
    monitorPtr->eventFlags &= ~filteredEvents;
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
{
    fdMon_t *monitorPtr;

    // Look up the File Descriptor Monitor object using the safe reference provided.
    // Note that the safe reference map is shared by all threads in the process, so it
    // must be protected using the mutex.  The File Descriptor Monitor objects, on the other
    // hand, are only allowed to be accessed by the one thread that created them, so it is
    // safe to unlock the mutex after doing the safe reference lookup.
    LOCK
    monitorPtr = le_ref_Lookup(FdMonitorRefMap, monitorRef);
    UNLOCK

    LE_FATAL_IF(monitorPtr == NULL, "File Descriptor Monitor %p doesn't exist!", monitorRef);
    LE_FATAL_IF(thread_GetEventRecPtr() != monitorPtr->threadRecPtr,
                "FD Monitor '%s' (fd %d) is owned by another thread.",
                FDMON_NAME(monitorPtr->name),
                monitorPtr->fd);

    fa_fdMon_SetDeferrable(monitorPtr, isDeferrable);
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
{
    fdMon_t *monitorPtr;

    // Look up the File Descriptor Monitor object using the safe reference provided.
    // Note that the safe reference map is shared by all threads in the process, so it
    // must be protected using the mutex.  The File Descriptor Monitor objects, on the other
    // hand, are only allowed to be accessed by the one thread that created them, so it is
    // safe to unlock the mutex after doing the safe reference lookup.
    LOCK
    monitorPtr = le_ref_Lookup(FdMonitorRefMap, monitorRef);
    UNLOCK

    LE_FATAL_IF(monitorPtr == NULL, "File Descriptor Monitor %p doesn't exist!", monitorRef);
    LE_FATAL_IF(thread_GetEventRecPtr() != monitorPtr->threadRecPtr,
                "FD Monitor '%s' (fd %d) is owned by another thread.",
                FDMON_NAME(monitorPtr->name),
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
void *le_fdMonitor_GetContextPtr
(
    void
)
{
    // Fetch the pointer to the FD Monitor from thread-specific data.
    fdMon_t *monitorPtr = pthread_getspecific(FDMonitorPtrKey);

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
{
    // Fetch the pointer to the FD Monitor from thread-specific data.
    fdMon_t* monitorPtr = pthread_getspecific(FDMonitorPtrKey);

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
{
    fdMon_t *monitorPtr;

    LOCK
    monitorPtr = le_ref_Lookup(FdMonitorRefMap, monitorRef);
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
{
    fdMon_t *monitorPtr;

    LOCK
    monitorPtr = le_ref_Lookup(FdMonitorRefMap, monitorRef);
    UNLOCK

    LE_FATAL_IF(monitorPtr == NULL, "FD Monitor object %p does not exist!", monitorRef);

    DeleteFdMonitor(monitorPtr);
}
