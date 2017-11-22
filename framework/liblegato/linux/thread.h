//--------------------------------------------------------------------------------------------------
/** @file thread.h
 *
 * This module contains initialization functions for the legato thread system that should be called
 * by the build system.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#ifndef THREAD_INCLUDE_GUARD
#define THREAD_INCLUDE_GUARD

#include "eventLoop.h"
#include "mutex.h"
#include "semaphores.h"
#include "timer.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum thread name size in bytes.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_THREAD_NAME_SIZE        24


//--------------------------------------------------------------------------------------------------
/**
 * The legato thread structure containing all of the thread's attributes.
 *
 * @note    A Thread object created using le_thread_InitLegatoThreadData() will have its mainFunc
 *          set to NULL, and will not be joinable using le_thread_Join(), regardless of the thread's
 *          actual detach state.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t           link;           ///< Link for exposure to the Inpsect tool.
    char    name[MAX_THREAD_NAME_SIZE];     ///< The name of the thread.
    pthread_attr_t          attr;           ///< The thread's attributes.
    le_thread_Priority_t    priority;       ///< The thread's priority.
    bool                    isJoinable;     ///< true = the thread is joinable, false = detached.

    /// Thread state.
    enum
    {
        THREAD_STATE_NEW,       ///< Not yet started.
        THREAD_STATE_RUNNING,   ///< Has been started.
        THREAD_STATE_DYING      ///< Is in the process of cleaning up.
    }
    state;

    le_thread_MainFunc_t    mainFunc;                       ///< The main function for the thread.
    void*                   context;                        ///< Context value to be passed to mainFunc.
    le_dls_List_t           destructorList;                 ///< The destructor list for this thread.
    mutex_ThreadRec_t       mutexRec;                       ///< The thread's mutex record.
    sem_ThreadRec_t         semaphoreRec;                   ///< The thread's semaphore record.
    event_PerThreadRec_t    eventRec;                       ///< The thread's event record.
    pthread_t               threadHandle;                   ///< The pthreads thread handle.
    le_thread_Ref_t         safeRef;                        ///< Safe reference for this object.
    timer_ThreadRec_t       timerRec[TIMER_TYPE_COUNT];     ///< The thread's timer records.
}
thread_Obj_t;


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the thread obj list; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t* thread_GetThreadObjList
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the thread obj list change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t** thread_GetThreadObjListChgCntRef
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the thread system.  This function must be called before any other thread functions
 * are called.
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
void thread_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Perform thread specific initialization for the current thread
 */
//--------------------------------------------------------------------------------------------------
void thread_InitThread
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the calling thread's mutex record.
 */
//--------------------------------------------------------------------------------------------------
mutex_ThreadRec_t* thread_GetMutexRecPtr
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the calling thread's semaphore record.
 */
//--------------------------------------------------------------------------------------------------
sem_ThreadRec_t* thread_GetSemaphoreRecPtr
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the calling thread's event record.
 */
//--------------------------------------------------------------------------------------------------
event_PerThreadRec_t* thread_GetEventRecPtr
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets another thread's event record.
 */
//--------------------------------------------------------------------------------------------------
event_PerThreadRec_t* thread_GetOtherEventRecPtr
(
    le_thread_Ref_t threadRef
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the specified calling thread's timer record.
 */
//--------------------------------------------------------------------------------------------------
timer_ThreadRec_t* thread_GetTimerRecPtr
(
    timer_Type_t timerType
);


#endif  // THREAD_INCLUDE_GUARD
