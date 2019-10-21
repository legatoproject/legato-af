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

#include "fa/eventLoop.h"

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

#endif // LEGATO_LIBLEGATO_EVENTLOOP_H_INCLUDE_GUARD
