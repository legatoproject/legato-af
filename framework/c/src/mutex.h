/** @file mutex.h
 * 
 * Mutex module's intra-framework header file.  This file exposes type definitions and function
 * interfaces to other modules inside the framework implementation.
 * 
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_SRC_MUTEX_H_INCLUDE_GUARD
#define LEGATO_SRC_MUTEX_H_INCLUDE_GUARD

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
    le_mutex_Ref_t  waitingOnMutex;     ///< Reference to the mutex that is being waited on.
    le_dls_List_t   lockedMutexList;    ///< List of mutexes currently held by this thread.
    le_dls_Link_t   waitingListLink;    ///< Used to link into Mutex object's waiting list.
}
mutex_ThreadRec_t;


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
