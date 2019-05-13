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

//--------------------------------------------------------------------------------------------------
/** Fallback definition of EPOLLWAKEUP
 *
 * Definition of EPOLLWAKEUP for kernel versions that do not support it.
 */
//--------------------------------------------------------------------------------------------------
#ifndef EPOLLWAKEUP
#   pragma message "EPOLLWAKEUP unsupported. Power management features may fail."
#   define EPOLLWAKEUP 0x0
#endif

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
typedef struct
{
    fdMon_t     base;           ///< Base FD monitor.
    uint32_t    epollEvents;    ///< Flags for epoll(7).
    bool        isAlwaysReady;  ///< Don't use epoll(7).  Treat as always ready.
}
fdMon_Linux_t;

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

//--------------------------------------------------------------------------------------------------
/**
 * Define static pool for fd monitor
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(FdMonitor, LE_CONFIG_MAX_FD_MONITOR_POOL_SIZE, sizeof(fdMon_Linux_t));

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
 * Tell epoll(7) to stop monitoring an FD Monitor object's fd.
 **/
//--------------------------------------------------------------------------------------------------
static void StopMonitoringFd
(
    fdMon_Linux_t *linuxMonPtr
)
//--------------------------------------------------------------------------------------------------
{
    event_LinuxPerThreadRec_t   *threadRecPtr = CONTAINER_OF(linuxMonPtr->base.threadRecPtr,
                                    event_LinuxPerThreadRec_t, portablePerThreadRec);

    if (linuxMonPtr->isAlwaysReady)
    {
        return;
    }

    TRACE("Deleting fd %d (%s) from thread's epoll set.",
          linuxMonPtr->base.fd,
          FDMON_NAME(linuxMonPtr->base.name));

    if (epoll_ctl(threadRecPtr->epollFd, EPOLL_CTL_DEL, linuxMonPtr->base.fd, NULL) == -1)
    {
        if (errno == EBADF)
        {
            LE_CRIT("epoll_ctl(DEL) resulted in EBADF.  Probably because fd %d was closed"
                        " before deleting FD Monitor '%s'.",
                    linuxMonPtr->base.fd,
                    FDMON_NAME(linuxMonPtr->base.name));
        }
        else if (errno == ENOENT)
        {
            LE_CRIT("epoll_ctl(DEL) resulted in ENOENT.  Probably because fd %d was closed"
                        " before deleting FD Monitor '%s'.",
                    linuxMonPtr->base.fd,
                    FDMON_NAME(linuxMonPtr->base.name));
        }
        else
        {
            LE_FATAL("epoll_ctl(DEL) failed for fd %d. errno = %d (%m). FD Monitor '%s'.",
                     linuxMonPtr->base.fd,
                     errno,
                     FDMON_NAME(linuxMonPtr->base.name));
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Update the epoll(7) FD for a given FD Monitor object.
 **/
//--------------------------------------------------------------------------------------------------
static void UpdateEpollFd
(
    fdMon_Linux_t *linuxMonPtr
)
{
    event_LinuxPerThreadRec_t   *threadRecPtr;
    struct epoll_event           ev;

    if (linuxMonPtr->isAlwaysReady)
    {
        return;
    }

    memset(&ev, 0, sizeof(ev));
    ev.events = linuxMonPtr->epollEvents;
    ev.data.ptr = linuxMonPtr->base.safeRef;

    threadRecPtr = CONTAINER_OF(linuxMonPtr->base.threadRecPtr, event_LinuxPerThreadRec_t,
                                portablePerThreadRec);

    if (epoll_ctl(threadRecPtr->epollFd, EPOLL_CTL_MOD, linuxMonPtr->base.fd, &ev) == -1)
    {
        if (errno == EBADF)
        {
            LE_FATAL("epoll_ctl(MOD) resulted in EBADF.  Probably because fd %d was closed"
                     " before deleting FD Monitor '%s'.",
                  linuxMonPtr->base.fd,
                  FDMON_NAME(linuxMonPtr->base.name));
        }
        else
        {
            LE_FATAL(
                "epoll_ctl(MOD) failed for fd %d and epoll events %x on monitor '%s' with error %d",
                linuxMonPtr->base.fd, linuxMonPtr->epollEvents, FDMON_NAME(linuxMonPtr->base.name),
                errno);
        }
    }
}

// ==============================================
//  FRAMEWORK ADAPTOR FUNCTIONS
// ==============================================

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the FD Monitor module.
 *
 * This function must be called exactly once at process start-up, before any other FD Monitor
 * functions are called.
 *
 * @return The memory pool from which to allocate FD monitor instances.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t fa_fdMon_Init
(
    void
)
{
    // Get a reference to the trace keyword that is used to control tracing in this module.
    TraceRef = le_log_GetTraceRef("fdMonitor");

    return le_mem_InitStaticPool(FdMonitor, LE_CONFIG_MAX_FD_MONITOR_POOL_SIZE,
        sizeof(fdMon_Linux_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete an FD Monitor object for a given thread.  This frees resources associated with the
 * monitor, but not the monitor instance itself.
 */
//--------------------------------------------------------------------------------------------------
void fa_fdMon_Delete
(
    fdMon_t *fdMonitorPtr ///< [in] Pointer to the FD Monitor.
)
{
    fdMon_Linux_t *linuxMonPtr = CONTAINER_OF(fdMonitorPtr, fdMon_Linux_t, base);

    // Tell epoll(7) to stop monitoring this fd.
    StopMonitoringFd(linuxMonPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Dispatch an FD Event to the appropriate registered handler function.
 */
//--------------------------------------------------------------------------------------------------
void fa_fdMon_DispatchToHandler
(
    fdMon_t     *fdMonitorPtr,  ///< FD Monitor instance.
    uint32_t     flags          ///< Event flags.
)
{
    fdMon_Linux_t *linuxMonPtr = CONTAINER_OF(fdMonitorPtr, fdMon_Linux_t, base);

    // Mask out any events that have been disabled since epoll_wait() reported these events to us.
    flags &= (fdMonitorPtr->eventFlags | POLLERR | POLLHUP | POLLRDHUP);

    // If there's nothing left to report to the handler, don't call it.
    if (flags == 0)
    {
        // Note: if the fd is always ready to read or write (is not supported by epoll()), then
        //       we will only end up in here if both POLLIN and POLLOUT are disabled, in which case
        //       returning now will prevent re-queuing of DispatchToHandler(), which is what we
        //       want.  When either POLLIN or POLLOUT are re-enabled, le_fdMonitor_Enable() will
        //       call fdMon_Report() to get things going again.
        return;
    }

    if (IS_TRACE_ENABLED())
    {
        char eventsTextBuff[128];

        TRACE("Calling event handler for FD Monitor %s (fd %d, events %s).",
              FDMON_NAME(fdMonitorPtr->name),
              fdMonitorPtr->fd,
              fdMon_GetEventsText(eventsTextBuff, sizeof(eventsTextBuff), flags));
    }

    // Call the handler function.
    fdMonitorPtr->handlerFunc(fdMonitorPtr->fd, flags);

    // If this fd is always ready (is not supported by epoll) and either POLLIN or POLLOUT
    // are enabled, then queue up another dispatcher for this FD Monitor.
    // If neither are enabled, then le_fdMonitor_Enable() will queue the dispatcher
    // when one of them is re-enabled.
    if ((linuxMonPtr->isAlwaysReady) && (fdMonitorPtr->eventFlags & (POLLIN | POLLOUT)))
    {
        fdMon_Report(fdMonitorPtr->safeRef, fdMonitorPtr->eventFlags & (POLLIN | POLLOUT));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the platform-specific part of an FD monitor instance.
 *
 * @note The global monitor mutex will be locked for the duration of this function.
 */
//--------------------------------------------------------------------------------------------------
void fa_fdMon_Create
(
    fdMon_t *fdMonitorPtr ///< Monitor instance to initialize.
)
{
    event_LinuxPerThreadRec_t   *linuxEventRecPtr = CONTAINER_OF(thread_GetEventRecPtr(),
                                    event_LinuxPerThreadRec_t, portablePerThreadRec);
    fdMon_Linux_t               *linuxMonPtr = CONTAINER_OF(fdMonitorPtr, fdMon_Linux_t, base);
    struct epoll_event           ev;
    uint32_t                     pollEvents;

    // Non-deferrable by default.
    linuxMonPtr->epollEvents = PollToEPoll(fdMonitorPtr->eventFlags) | EPOLLWAKEUP;
    linuxMonPtr->isAlwaysReady = false;

    // Tell epoll(7) to start monitoring this fd.
    memset(&ev, 0, sizeof(ev));
    ev.events = linuxMonPtr->epollEvents;
    ev.data.ptr = fdMonitorPtr->safeRef;
    if (epoll_ctl(linuxEventRecPtr->epollFd, EPOLL_CTL_ADD, fdMonitorPtr->fd, &ev) == -1)
    {
        if (errno == EPERM)
        {
            LE_DEBUG("fd %d doesn't support epoll(), assuming always readable and writeable.",
                fdMonitorPtr->fd);
            linuxMonPtr->isAlwaysReady = true;

            // If either POLLIN or POLLOUT are enabled, queue up the handler for this now.
            pollEvents = fdMonitorPtr->eventFlags & (POLLIN | POLLOUT);
            if (pollEvents != 0)
            {
                fdMon_Report(fdMonitorPtr->safeRef, pollEvents);
            }
        }
        else
        {
            LE_FATAL("epoll_ctl(ADD) failed for fd %d with error %d", fdMonitorPtr->fd, errno);
        }
    }
}

///--------------------------------------------------------------------------------------------------
/**
 * Enable monitoring for events on a file descriptor.
 *
 * @return Events filtered for those that can be enabled.
 */
//--------------------------------------------------------------------------------------------------
short fa_fdMon_Enable
(
    fdMon_t *monitorPtr,        ///< FD monitor for which events are being enabled.
    fdMon_t *handlerMonitorPtr, ///< FD monitor for the handler which is calling the enable
                                ///< function.
                                ///< May be NULL if not enabling from a handler.
    short    events             ///< Bitmap of events.
)
{
    fdMon_Linux_t   *linuxMonPtr = CONTAINER_OF(monitorPtr, fdMon_Linux_t, base);
    short            filteredEvents = events & (POLLIN | POLLOUT | POLLPRI);
    uint32_t         epollEvents = PollToEPoll(filteredEvents);

    // If the fd doesn't support epoll, we assume it is always ready for read and write.
    // As long as EPOLLIN or EPOLLOUT (or both) is enabled for one of these fds, DispatchToHandler()
    // keeps re-queueing itself to the thread's event queue.  But it will stop doing that if
    // EPOLLIN and EPOLLOUT are both disabled.  So, here is where we get things going again when
    // EPOLLIN or EPOLLOUT is enabled outside DispatchToHandler() for that fd.
    if ( (linuxMonPtr->isAlwaysReady)
        && (epollEvents & (EPOLLIN | EPOLLOUT))
        && ((linuxMonPtr->epollEvents & (EPOLLIN | EPOLLOUT)) == 0) )
    {
        // If no handler is running or some other fd's handler is running,
        if ((handlerMonitorPtr == NULL) ||
            (handlerMonitorPtr->safeRef == monitorPtr->safeRef))
        {
            // Queue up DispatchToHandler() for this fd.
            fdMon_Report(monitorPtr->safeRef, filteredEvents & (POLLIN | POLLOUT));
        }
    }

    // Bit-wise OR the newly enabled event flags into the FD Monitor's epoll(7) flags set.
    linuxMonPtr->epollEvents |= epollEvents;
    UpdateEpollFd(linuxMonPtr);

    return filteredEvents;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable monitoring for events on a file descriptor.
 *
 * @return Events filtered for those that can be disabled.
 */
//--------------------------------------------------------------------------------------------------
short fa_fdMon_Disable
(
    fdMon_t *monitorPtr,        ///< FD monitor for which events are being disabled.
    fdMon_t *handlerMonitorPtr, ///< FD monitor for the handler which is calling the disable
                                ///< function.  May be NULL if not disabling from a handler.
    short    events             ///< Bitmap of events.
)
{
    fdMon_Linux_t   *linuxMonPtr = CONTAINER_OF(monitorPtr, fdMon_Linux_t, base);
    short            filteredEvents = events & (POLLIN | POLLOUT | POLLPRI);

    // Convert the events from POLLxx events to EPOLLxx events.
    uint32_t         epollEvents = PollToEPoll(filteredEvents);

    LE_UNUSED(handlerMonitorPtr);

    // Remove them from the FD Monitor's epoll(7) flags set.
    linuxMonPtr->epollEvents &= ~epollEvents;
    UpdateEpollFd(linuxMonPtr);

    return filteredEvents;
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
void fa_fdMon_SetDeferrable
(
    fdMon_t *monitorPtr,  ///< FD monitor instance.
    bool     isDeferrable ///< Deferrable (true) or urgent (false).
)
{
    fdMon_Linux_t *linuxMonPtr = CONTAINER_OF(monitorPtr, fdMon_Linux_t, base);

    // Set/clear the EPOLLWAKEUP flag in the FD Monitor's epoll(7) flags set.
    if (isDeferrable)
    {
        linuxMonPtr->epollEvents &= ~EPOLLWAKEUP;
    }
    else
    {
        linuxMonPtr->epollEvents |= EPOLLWAKEUP;
    }

    UpdateEpollFd(linuxMonPtr);
}
