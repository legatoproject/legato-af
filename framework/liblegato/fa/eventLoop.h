/**
 * @file eventLoop.h
 *
 * Event loop interface that must be implemented by a Legato framework adaptor.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef FA_EVENTLOOP_H_INCLUDE_GUARD
#define FA_EVENTLOOP_H_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of the possible states that a thread's Event Loop can be in.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_EVENT_LOOP_INITIALIZED,      ///< Initialized, but not running yet.
    LE_EVENT_LOOP_RUNNING,          ///< le_event_RunLoop() has been called.
    LE_EVENT_LOOP_DESTRUCTED,       ///< Event loop destructed (thread is shutting down).
}
event_LoopState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Event Loop's per-thread record.
 *
 * One of these must be allocated as a member of the Thread object.  The Event Loop module
 * will call the function thread_GetEventRecPtr() to fetch a pointer to it.
 *
 * @warning No code outside of the Event Loop module or the FD Monitor module should ever access
 * any member of this structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sls_List_t        eventQueue;        ///< The thread's event queue.
    le_dls_List_t        handlerList;       ///< List of handlers registered with this thread.
    le_dls_List_t        fdMonitorList;     ///< List of FD Monitors created by this thread.
    void                *contextPtr;        ///< Context pointer from last Handler called.
    event_LoopState_t    state;             ///< Current state of the event loop.
    uint64_t             liveEventCount;    ///< Number of events ready for dequeing.  Ensures
                                            ///< balance between queued events and monitored fds
                                            ///< in le_event_ServiceLoop().
}
event_PerThreadRec_t;

//--------------------------------------------------------------------------------------------------
/**
 * Initialize any platform-specific Event Loop info.
 */
//--------------------------------------------------------------------------------------------------
void fa_event_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Destruct the Event Loop for a given thread.
 *
 * This function is called when destructing a thread to clean up per-OS thread-specific info.
 */
//--------------------------------------------------------------------------------------------------
void fa_event_DestructThread
(
    event_PerThreadRec_t *portablePerThreadRecPtr ///< [IN] Per-thread event record
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the platform-specific Event Loop info for a given thread.
 *
 * The actual allocation is done here so the framework adaptor can allocate extra space for
 * os-specific info.  Common info does not need to be initialized as it will be initialized
 * by event_CreatePerThreadInfo()
 */
//--------------------------------------------------------------------------------------------------
event_PerThreadRec_t *fa_event_CreatePerThreadInfo
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize any platform-specific per-thread Event Loop info.
 */
//--------------------------------------------------------------------------------------------------
void fa_event_ThreadInit
(
    event_PerThreadRec_t *perThreadRecPtr ///< [IN] Per-thread event record
);

//--------------------------------------------------------------------------------------------------
/**
 * Inform event loop an event has fired.  Wakes the event loop if it is asleep.
 */
//--------------------------------------------------------------------------------------------------
void fa_event_TriggerEvent_NoLock
(
    event_PerThreadRec_t *perThreadRecPtr ///< [IN] Per-thread event record
);

//--------------------------------------------------------------------------------------------------
/**
 * Wait for an event to trigger.  This fetches the value of the Event FD (which is
 * the number of event reports on the Event Queue) and resets the Event FD value to zero.
 *
 * @return The number of Event Reports on the thread's Event Queue.
 */
//--------------------------------------------------------------------------------------------------
uint64_t fa_event_WaitForEvent
(
    event_PerThreadRec_t *perThreadRecPtr ///< [IN] Per-thread event record
);

//--------------------------------------------------------------------------------------------------
/**
 * Runs the event loop for the calling thread.
 *
 * This starts processing events by the calling thread.
 *
 * Can only be called once for each thread, and must never be called in
 * the process's main thread.
 *
 * @note
 *      Function never returns.
 */
//--------------------------------------------------------------------------------------------------
void fa_event_RunLoop
(
    void
)
__attribute__ ((noreturn));

#endif /* end FA_EVENTLOOP_H_INCLUDE_GUARD */
