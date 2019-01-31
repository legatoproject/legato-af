/** @file mutex.h
 *
 * Mutex module's intra-framework header file.  This file exposes type definitions and function
 * interfaces to other modules inside the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SRC_MUTEX_H_INCLUDE_GUARD
#define LEGATO_SRC_MUTEX_H_INCLUDE_GUARD

/// Maximum number of bytes in a mutex name (including null terminator).
#define MAX_NAME_BYTES 24

//--------------------------------------------------------------------------------------------------
/**
 * Mutex object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mutex
{
    le_dls_Link_t       mutexListLink;      ///< Used to link onto the process's Mutex List.
    le_thread_Ref_t     lockingThreadRef;   ///< Reference to the thread that holds the lock.
#if LE_CONFIG_LINUX_TARGET_TOOLS
    le_dls_Link_t       lockedByThreadLink; ///< Used to link onto the thread's locked mutexes list.
    le_dls_List_t       waitingList;        ///< List of threads waiting for this mutex.
    pthread_mutex_t     waitingListMutex;   ///< Pthreads mutex used to protect the waiting list.
#endif
    bool                isRecursive;        ///< true if recursive, false otherwise.
    int                 lockCount;      ///< Number of lock calls not yet matched by unlock calls.
    pthread_mutex_t     mutex;          ///< Pthreads mutex that does the real work. :)
#if LE_CONFIG_MUTEX_NAMES_ENABLED
    char                name[MAX_NAME_BYTES]; ///< The name of the mutex (UTF8 string).
#endif
}
Mutex_t;


//--------------------------------------------------------------------------------------------------
/**
 * Mutex Thread Record.
 *
 * This structure is to be stored as a member in each Thread object.  The function
 * thread_GetMutexRecPtr() is used by the mutex module to fetch a pointer to one of these records
 * for a given thread.
 *
 * @warning No code outside of the mutex module (mutex.c) should ever access the members of this
 *          structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
#if LE_CONFIG_LINUX_TARGET_TOOLS
    le_mutex_Ref_t  waitingOnMutex;     ///< Reference to the mutex that is being waited on.
    le_dls_List_t   lockedMutexList;    ///< List of mutexes currently held by this thread.
    le_dls_Link_t   waitingListLink;    ///< Used to link into Mutex object's waiting list.
#endif
}
mutex_ThreadRec_t;


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the mutex list change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t** mutex_GetMutexListChgCntRef
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Mutex module.
 *
 * This function must be called exactly once at process start-up before any other mutex module
 * functions are called.
 */
//--------------------------------------------------------------------------------------------------
void mutex_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the thread-specific parts of the mutex module.
 *
 * This function must be called once by each thread when it starts, before any other mutex module
 * functions are called by that thread.
 */
//--------------------------------------------------------------------------------------------------
void mutex_ThreadInit
(
    void
);


#endif /* LEGATO_SRC_MUTEX_H_INCLUDE_GUARD */
