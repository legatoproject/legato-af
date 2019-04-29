//--------------------------------------------------------------------------------------------------
/** @file eventLoop.c
 *
 * Legato @ref c_linuxEventLoop implementation for Linux
 *
 * @section eventLoop_LinuxImplementation    Linux Event Loop Implementation
 *
 * There are two types of Event Report that can be added to an Event Queue:
 *
 *  - Queued Function
 *  - Publish-Subscribe Event Report - Different size objects, depending on what payload they
 *                                     carry.
 *
 * All the different types of Event Report all have the same base structure.  Their payload
 * is different, though.
 *
 * The Event Loop for each thread uses an epoll fd to test for events (see 'man epoll').
 *
 * Included in the set of file descriptors that are being monitored by epoll is an eventfd
 * (see 'man eventfd') monitored in "level-triggered" mode.
 *
 * Whenever an Event Report is added to the Event Queue for a thread, the number 1 is written to
 * that thread's eventfd.  When Event Reports are popped off a thread's Event Queue, that thread's
 * eventfd is read to decrement it.  As long as the eventfd's value is greater than 0, epoll_wait()
 * will return immediately, reporting that there is something to read from that fd.
 *
 * The Event Loop is an infinite loop that calls epoll_wait() and then responds to any fd events
 * that epoll_wait() reports.  If epoll_wait() reports an event on the eventfd, then an Event Report
 * is popped off the Event Queue and processed.  If epoll_wait() reports an event on any other fd,
 * FD Event Reports are created and pushed onto Event Queues according to what handlers are
 * registered for those events.  All pending Event Reports are processed until the Event Queue is
 * empty before returning to epoll_wait().  (NOTE: This choice was made to save system call
 * overhead in times of heavy load.  Unfortunately, it also means that if event handlers always add
 * new events to the queue, then epoll_wait() will never be called and therefore fd events will
 * never be detected.)
 *
 * ----
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "eventLoop.h"
#include "thread.h"
#include "fdMonitor.h"
#include "limit.h"
#include "fileDescriptor.h"

#include <sys/eventfd.h>

// ==============================================
//  PRIVATE DATA
// ==============================================

/// Maximum number of events that can be received from epoll_wait() at one time.
#define MAX_EPOLL_EVENTS 32

//--------------------------------------------------------------------------------------------------
/**
 * Pool from which Per-Thread info are allocated
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PerThreadPool;

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

// ==============================================
//  FRAMEWORK ADAPTOR FUNCTIONS
// ==============================================

//--------------------------------------------------------------------------------------------------
/**
 * Initialize platform-specific info.
 *
 */
//--------------------------------------------------------------------------------------------------
void fa_event_Init
(
    void
)
{
    // Create the pool from which Linux-specific thread record objects are allocated.
    PerThreadPool = le_mem_CreatePool("PerThreadEvent", sizeof(event_LinuxPerThreadRec_t));
    le_mem_ExpandPool(PerThreadPool, LE_CONFIG_MAX_THREAD_POOL_SIZE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the platform-specific Event Loop info for a given thread.
 *
 * The actual allocation is done here so the framework adaptor can allocate extra space for
 * os-specific info.  Common info does not need to be initialized as it will be initialized
 * by event_CreatePerThreadInfo()
 */
//--------------------------------------------------------------------------------------------------
event_PerThreadRec_t* fa_event_CreatePerThreadInfo
(
    void
)
{
    event_LinuxPerThreadRec_t* recPtr = le_mem_ForceAlloc(PerThreadPool);

    // Create the epoll file descriptor for this thread.  This will be used to monitor for
    // events on various file descriptors.
    recPtr->epollFd = epoll_create1(0);
    LE_FATAL_IF(recPtr->epollFd < 0, "epoll_create1(0) failed with errno %d.", errno);

    // Open an eventfd for this thread.  This will be uses to signal to the epoll fd that there
    // are Event Reports on the Event Queue.
    recPtr->eventQueueFd = eventfd(0, 0);
    LE_FATAL_IF(recPtr->eventQueueFd < 0, "eventfd() failed with errno %d.", errno);

    // Add the eventfd to the list of file descriptors to wait for using epoll_wait().
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLWAKEUP;
    ev.data.ptr = NULL;     // This being set to NULL is what tells the main event loop that this
                            // is the Event Queue FD, rather than another FD that is being
                            // monitored.
    if (epoll_ctl(recPtr->epollFd, EPOLL_CTL_ADD, recPtr->eventQueueFd, &ev) == -1)
    {
        LE_FATAL(   "epoll_ctl(ADD) failed for fd %d. errno = %d",
                    recPtr->eventQueueFd,
                    errno);
    }

    return &recPtr->portablePerThreadRec;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize any platform-specific per-thread Event Loop info.
 */
//--------------------------------------------------------------------------------------------------
void fa_event_ThreadInit
(
    event_PerThreadRec_t* perThreadRecPtr
)
{
    // Nothing needed on Linux
}


//--------------------------------------------------------------------------------------------------
/**
 * Destruct the Event Loop for a given thread.
 *
 * This function is called exactly once at thread shutdown from event_DestructThread().
 */
//--------------------------------------------------------------------------------------------------
void fa_event_DestructThread
(
    event_PerThreadRec_t* portablePerThreadRecPtr
)
{
    event_LinuxPerThreadRec_t* perThreadRecPtr =
        CONTAINER_OF(portablePerThreadRecPtr,
                     event_LinuxPerThreadRec_t,
                     portablePerThreadRec);

    // Close the epoll file descriptor.
    fd_Close(perThreadRecPtr->epollFd);

    // Close the eventfd for the Event Queue.
    fd_Close(perThreadRecPtr->eventQueueFd);

    le_mem_Release(perThreadRecPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a thread's Event File Descriptor.  This increments it by one.
 *
 * This must be done exactly once for each Event Report pushed onto the thread's Event Queue.
 */
//--------------------------------------------------------------------------------------------------
void fa_event_TriggerEvent_NoLock
(
    event_PerThreadRec_t* portablePerThreadRecPtr
)
{
    static const uint64_t writeBuff = 1;    // Write the value 1 to increment the eventfd by 1.
    event_LinuxPerThreadRec_t* perThreadRecPtr =
        CONTAINER_OF(portablePerThreadRecPtr,
                     event_LinuxPerThreadRec_t,
                     portablePerThreadRec);

    ssize_t writeSize;

    for (;;)
    {
        writeSize = write(perThreadRecPtr->eventQueueFd, &writeBuff, sizeof(writeBuff));
        if (writeSize == sizeof(writeBuff))
        {
            return;
        }
        else
        {
            if ((writeSize == -1) && (errno != EINTR))
            {
                LE_FATAL("write() failed with errno %d.", errno);
            }
            else
            {
                LE_FATAL("write() returned %zd! (expected %zd)", writeSize, sizeof(writeBuff));
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Read a thread's Event File Descriptor.  This fetches the value of the Event FD (which is
 * the number of event reports on the Event Queue) and resets the Event FD value to zero.
 *
 * @return The number of Event Reports on the thread's Event Queue.
 */
//--------------------------------------------------------------------------------------------------
uint64_t fa_event_WaitForEvent
(
    event_PerThreadRec_t* portablePerThreadRecPtr
)
{
    uint64_t readBuff;
    ssize_t readSize;
    event_LinuxPerThreadRec_t* perThreadRecPtr =
        CONTAINER_OF(portablePerThreadRecPtr,
                     event_LinuxPerThreadRec_t,
                     portablePerThreadRec);

    for (;;)
    {
        readSize = read(perThreadRecPtr->eventQueueFd, &readBuff, sizeof(readBuff));
        if (readSize == sizeof(readBuff))
        {
            return readBuff;
        }
        else
        {
            if ((readSize == -1) && (errno != EINTR))
            {
                LE_FATAL("read() failed with errno %d.", errno);
            }
            else
            {
                LE_FATAL("read() returned %zd! (expected %zd)", readSize, sizeof(readBuff));
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Runs the event loop for the calling thread.
 *
 * This starts the processing of events by the calling thread.
 *
 * This function can only be called at most once for each thread, and must never be called in
 * the process's main thread.
 *
 * @note
 *      This function never returns.
 */
//--------------------------------------------------------------------------------------------------
void fa_event_RunLoop
(
    void
)
{
    event_PerThreadRec_t* perThreadRecPtr = thread_GetEventRecPtr();

    int epollFd = CONTAINER_OF(perThreadRecPtr,
                               event_LinuxPerThreadRec_t,
                               portablePerThreadRec)->epollFd;
    struct epoll_event epollEventList[MAX_EPOLL_EVENTS];

    // Make sure nobody calls this function more than once in the same thread.
    LE_ASSERT(perThreadRecPtr->state == LE_EVENT_LOOP_INITIALIZED);

    // Update the state of the Event Loop.
    perThreadRecPtr->state = LE_EVENT_LOOP_RUNNING;

    // Enter the infinite loop itself.
    for (;;)
    {
        // Wait for something to happen on one of the file descriptors that we are monitoring
        // using our epoll fd.
        int result = epoll_wait(epollFd, epollEventList, NUM_ARRAY_MEMBERS(epollEventList), -1);

        // If something happened on one or more of the monitored file descriptors,
        if (result > 0)
        {
            int i;

            // Check if someone has cancelled the thread and terminate the thread now, if so.
            pthread_testcancel();

            // For each fd event reported by epoll_wait(), if it is any file descriptor other
            // than the eventfd (which is used to indicate that there is something on the
            // Event Queue), queue an Event Report to the Event Queue for that fd.
            for (i = 0; i < result; i++)
            {
                // Get the pointer that we registered with epoll_ctl(2) along with this fd.
                // The value of this pointer will either be NULL or a Safe Reference for an
                // FD Monitor object.  If it is NULL, then the Event Queue's eventfd is the
                // fd that experienced the event.
                void* safeRef = epollEventList[i].data.ptr;

                if (safeRef != NULL)
                {
                    fdMon_Report(safeRef, EPollToPoll(epollEventList[i].events));
                }
            }

            // Process all the Event Reports on the Event Queue.
            event_ProcessEventReports(perThreadRecPtr);
        }
        // Otherwise, if an epoll_wait() reported an error, hopefully it's just an interruption
        // by a signal (EINTR).  Anything else is a fatal error.
        else if (result < 0)
        {
            if (errno != EINTR)
            {
                LE_FATAL("epoll_wait() failed.  errno = %d.", errno);
            }

            // It was just EINTR, so we are okay to go back to sleep.  But first,
            // check if someone has cancelled the thread and terminate the thread now, if so.
            pthread_testcancel();
        }
        // Otherwise, if epoll_wait() returned zero, something has gone horribly wrong, because
        // it should never return zero.
        else
        {
            LE_FATAL("epoll_wait() returned zero!");
        }
    }
}

// ==============================================
//  PUBLIC API FUNCTIONS
// ==============================================

//--------------------------------------------------------------------------------------------------
/**
 * Fetches a file descriptor that will appear readable to poll() and select() when the calling
 * thread's Event Loop needs servicing (via a call to le_event_ServiceLoop()).
 *
 * @warning This function is only intended for use when integrating with legacy POSIX-based software
 * that cannot be easily refactored to use the Legato Event Loop.  The preferred approach is
 * to call le_event_RunLoop().
 *
 * @return The file descriptor.
 */
//--------------------------------------------------------------------------------------------------
int le_event_GetFd
(
    void
)
{
    return CONTAINER_OF(thread_GetEventRecPtr(),
                        event_LinuxPerThreadRec_t,
                        portablePerThreadRec)->epollFd;
}


//--------------------------------------------------------------------------------------------------
/**
 * Services the calling thread's Event Loop.
 *
 * @warning This function is only intended for use when integrating with legacy POSIX-based software
 * that cannot be easily refactored to use the Legato Event Loop.  The preferred approach is
 * to call le_event_RunLoop().
 *
 * See also: le_event_GetFd().
 *
 * @return
 *  - LE_OK if there is more to be done.
 *  - LE_WOULD_BLOCK if there were no events to process.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_event_ServiceLoop
(
    void
)
{
    event_PerThreadRec_t* perThreadRecPtr = thread_GetEventRecPtr();
    int epollFd = CONTAINER_OF(perThreadRecPtr,
                               event_LinuxPerThreadRec_t,
                               portablePerThreadRec)->epollFd;
    struct epoll_event epollEventList[MAX_EPOLL_EVENTS];

    LE_DEBUG("perThreadRecPtr->liveEventCount is" "%" PRIu64, perThreadRecPtr->liveEventCount);

    // If there are still live events remaining in the queue, process a single event, then return
    if (perThreadRecPtr->liveEventCount > 0)
    {
        perThreadRecPtr->liveEventCount--;

        // This function assumes the mutex is NOT locked.
        event_ProcessOneEventReport(perThreadRecPtr);

        return LE_OK;
    }

    int result;

    do
    {
        // If no events on the queue, try to refill the event queue.
        // Ask epoll what, if anything, has happened on any of the file descriptors that we are
        // monitoring using our epoll fd.  (NOTE: This is non-blocking.)
        result = epoll_wait(epollFd, epollEventList, NUM_ARRAY_MEMBERS(epollEventList), 0);

        if ((result < 0) && (EINTR == errno))
        {
            // If epoll was interrupted,
            // Check if someone has cancelled the thread and terminate the thread now, if so.
            pthread_testcancel();
        }
    }
    while ((result < 0) && (EINTR == errno));

    // If something happened on one or more of the monitored file descriptors,
    if (result > 0)
    {
        int i;

        // Check if someone has cancelled the thread and terminate the thread now, if so.
        pthread_testcancel();

        // For each fd event reported by epoll_wait(), if it is any file descriptor other
        // than the eventfd (which is used to indicate that there is something on the
        // Event Queue), queue an Event Report to the Event Queue for that fd.
        for (i = 0; i < result; i++)
        {
            // Get the pointer that we registered with epoll_ctl(2) along with this fd.
            // The value of this pointer will either be NULL or a Safe Reference for an
            // FD Monitor object.  If it is NULL, then the Event Queue's eventfd is the
            // fd that experienced the event, which we will deal with later in this function.
            void* safeRef = epollEventList[i].data.ptr;

            if (safeRef != NULL)
            {
                fdMon_Report(safeRef, EPollToPoll(epollEventList[i].events));
            }
        }
    }
    // Otherwise, check if an epoll_wait() reported an error.
    // Interruptions are tested above, so this is always a fatal error.
    else if (result < 0)
    {
        LE_FATAL("epoll_wait() failed.  errno = %d.", errno);
    }
    // Otherwise, if epoll_wait() returned zero, then either this function was called without
    // waiting for the eventfd to be readable, or the eventfd was readable momentarily, but
    // something changed between the time the application code detected the readable condition
    // and now that made the eventfd not readable anymore.
    else
    {
        LE_DEBUG("epoll_wait() returned zero.");

        return LE_WOULD_BLOCK;
    }

    // Read the eventfd to reset it to zero so epoll stops telling us about it until more
    // are added.
    perThreadRecPtr->liveEventCount = fa_event_WaitForEvent(perThreadRecPtr);

    LE_DEBUG("perThreadRecPtr->liveEventCount is" "%" PRIu64, perThreadRecPtr->liveEventCount);

    // If events were read, process the top event
    if (perThreadRecPtr->liveEventCount > 0)
    {
        perThreadRecPtr->liveEventCount--;
        event_ProcessOneEventReport(perThreadRecPtr);

        return LE_OK;
    }
    else
    {
        return LE_WOULD_BLOCK;
    }
}
