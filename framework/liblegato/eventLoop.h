//--------------------------------------------------------------------------------------------------
/** @file eventLoop.h
 *
 * Legato Event Loop module inter-module include file.
 *
 * This file exposes interfaces that are for use by other modules inside the framework
 * implementation, but must not be used outside of the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_LIBLEGATO_EVENTLOOP_H_INCLUDE_GUARD
#define LEGATO_LIBLEGATO_EVENTLOOP_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Component Initialization Function.
 *
 * All component initialization functions must conform to this prototype
 * (no parameters, no return value).
 */
//--------------------------------------------------------------------------------------------------
typedef void (*event_ComponentInitFunc_t)(void);


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
    le_sls_List_t       eventQueue;         ///< The thread's event queue.
    le_dls_List_t       handlerList;        ///< List of handlers registered with this thread.
    le_dls_List_t       fdMonitorList;      ///< List of FD Monitors created by this thread.
    void*               contextPtr;         ///< Context pointer from last Handler called.
    event_LoopState_t   state;              ///< Current state of the event loop.
    uint64_t            liveEventCount;     ///< Number of events ready for dequeing.  Ensures
                                            ///< balance between queued events and monitored fds
                                            ///< in le_event_ServiceLoop().
}
event_PerThreadRec_t;


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Event Loop module.
 *
 * This function must be called exactly once at process start-up, before any other Event module
 * or Event Loop API functions are called.
 */
//--------------------------------------------------------------------------------------------------
void event_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Event Loop for a given thread.
 *
 * This function must be called exactly once at thread start-up, before any other Event module
 * or Event Loop API functions (other than event_Init() ) are called by that thread.
 *
 * @note    The process main thread must call event_Init() first, then event_CreatePerThreadInfo().
 */
//--------------------------------------------------------------------------------------------------
event_PerThreadRec_t* event_CreatePerThreadInfo
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Event Loop for a given thread.
 *
 * This function must be called exactly once at thread start-up, before any other Event module
 * or Event Loop API functions (other than event_Init() ) are called by that thread.  This
 * must be called in the thread -- thread_CreatePerThreadInfo is called in the parent thread.
 *
 * @note    The process main thread must call event_Init() first, then event_CreatePerThreadInfo().
 */
//--------------------------------------------------------------------------------------------------
void event_ThreadInit
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Defer the component initializer for later execution.
 *
 * This function must be called at process start-up, before le_event_Runloop() is called for the
 * main thread.
 *
 * It takes a pointer to a Component Initialization Functions to be called when the event loop is
 * started (i.e., when le_event_RunLoop() is called) for the process's main thread.
 */
//--------------------------------------------------------------------------------------------------
void event_QueueComponentInit
(
    const event_ComponentInitFunc_t func    /// The initialization function to call.
);


//--------------------------------------------------------------------------------------------------
/**
 * Destruct the Event Loop for a given thread.
 *
 * This function must be called exactly once at thread shutdown, after any other Event module
 * or Event Loop API functions are called by that thread, and before the Thread object is
 * deleted.
 */
//--------------------------------------------------------------------------------------------------
void event_DestructThread
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the context pointer for the currently running thread.
 *
 * This can later be retrieved using le_event_GetContextPtr() from within the same thread.
 */
//--------------------------------------------------------------------------------------------------
void event_SetCurrentContextPtr
(
    void* contextPtr    ///< [in] Context pointer value.
);



//--------------------------------------------------------------------------------------------------
/**
 * Process one event report from the calling thread's Event Queue.
 *
 * This is usually called from the framework adaptor implementation of le_event_RunLoop() and
 * le_event_ServiceLoop()
 *
 **/
//--------------------------------------------------------------------------------------------------
void event_ProcessOneEventReport
(
    event_PerThreadRec_t* perThreadRecPtr   ///< [in] Ptr to the calling thread's per-thread record.
);


//--------------------------------------------------------------------------------------------------
/**
 * Process Event Reports from the calling thread's Event Queue until the queue is empty.
 *
 * This is usually called from the framework adaptor implementation of le_event_RunLoop() and
 * le_event_ServiceLoop()
 */
//--------------------------------------------------------------------------------------------------
void event_ProcessEventReports
(
    event_PerThreadRec_t* perThreadRecPtr   ///< [in] Ptr to the calling thread's per-thread record.
);

//--------------------------------------------------------------------------------------------------
/**
 * Guards against thread cancellation and locks the mutex.
 *
 * @return Old state of cancelability.
 **/
//--------------------------------------------------------------------------------------------------
int event_Lock
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Unlocks the mutex and releases the thread cancellation guard created by Lock().
 **/
//--------------------------------------------------------------------------------------------------
void event_Unlock
(
    int restoreTo   ///< Old state of cancellability to be restored.
);

//--------------------------------------------------------------------------------------------------
/**
 * Wait for a condition to fire.
 *
 * The event lock must be held before calling this function.
 */
//--------------------------------------------------------------------------------------------------
void event_CondWait
(
    pthread_cond_t* cond        ///< [IN] Condition to wait for
);

//--------------------------------------------------------------------------------------------------
/**
 * Wait for a condition to fire until timeout occurs.
 *
 * The event lock must be held before calling this function.
 *
 * @return A value of zero upon successful completion, an error number otherwise.
 */
//--------------------------------------------------------------------------------------------------
int event_CondTimedWait
(
    pthread_cond_t* cond,           ///< [IN] Condition to wait for.
    const struct timespec* timePtr  ///< [IN] Time to wait for condition before returning.
);

//--------------------------------------------------------------------------------------------------
// Framework adaptor functions for eventLoop.
//--------------------------------------------------------------------------------------------------

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
    event_PerThreadRec_t* portablePerThreadRecPtr   ///< [IN] Per-thread event record
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
event_PerThreadRec_t* fa_event_CreatePerThreadInfo
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
    event_PerThreadRec_t* perThreadRecPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Inform event loop an event has fired.  Wakes the event loop if it is asleep.
 */
//--------------------------------------------------------------------------------------------------
void fa_eventLoop_TriggerEvent_NoLock
(
    event_PerThreadRec_t* perThreadRecPtr   ///< [IN] Per-thread event record
);


//--------------------------------------------------------------------------------------------------
/**
 * Wait for an event to trigger.  This fetches the value of the Event FD (which is
 * the number of event reports on the Event Queue) and resets the Event FD value to zero.
 *
 * @return The number of Event Reports on the thread's Event Queue.
 */
//--------------------------------------------------------------------------------------------------
uint64_t fa_eventLoop_WaitForEvent
(
    event_PerThreadRec_t* perThreadRecPtr   ///< [IN] Per-thread event record
);


#endif // LEGATO_LIBLEGATO_EVENTLOOP_H_INCLUDE_GUARD
