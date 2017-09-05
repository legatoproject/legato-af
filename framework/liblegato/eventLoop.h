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

#ifndef LEGATO_SRC_EVENTLOOP_H_INCLUDE_GUARD
#define LEGATO_SRC_EVENTLOOP_H_INCLUDE_GUARD


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
    int                 epollFd;            ///< epoll(7) file descriptor.
    int                 eventQueueFd;       ///< eventfd(2) file descriptor for the Event Queue.
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
 * @note    The process main thread must call event_Init() first, then event_InitThread().
 */
//--------------------------------------------------------------------------------------------------
void event_InitThread
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




#endif // LEGATO_SRC_EVENTLOOP_H_INCLUDE_GUARD
