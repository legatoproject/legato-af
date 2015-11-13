//--------------------------------------------------------------------------------------------------
/** @file spy.c
 *
 * Legato Inspection tool's spy, whose functions are called by the modules under inspection in
 * order to help the Inspection tool do its jobs.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Local reference to the list of mem pools.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t* ListOfPoolsRef;


//--------------------------------------------------------------------------------------------------
/**
 * Local reference to the list of thread objects.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t* ListOfThreadObjsRef;


//--------------------------------------------------------------------------------------------------
/**
 * A counter that increments every time a change is made to ListOfPools.
 */
//--------------------------------------------------------------------------------------------------
static size_t** ListOfPoolsChgCntRefRef;


//--------------------------------------------------------------------------------------------------
/**
 * A counter that increments every time a change is made to ListOfThreadObjs.
 */
//--------------------------------------------------------------------------------------------------
static size_t** ListOfThreadObjsChgCntRefRef;


//--------------------------------------------------------------------------------------------------
/**
 * A counter that increments every time a change is made to the timer list of the currently
 * inspected thread object.
 */
//--------------------------------------------------------------------------------------------------
static size_t** ListOfTimersChgCntRefRef;


//--------------------------------------------------------------------------------------------------
/**
 * A counter that increments every time a change is made to the mutex list of the currently
 * inspected thread object.
 */
//--------------------------------------------------------------------------------------------------
static size_t** ListOfMutexesChgCntRefRef;


//--------------------------------------------------------------------------------------------------
/**
 * A counter that increments every time a change is made to the semaphore of the currently
 * inspected thread object.
 */
//--------------------------------------------------------------------------------------------------
static size_t** ListOfSemaphoresChgCntRefRef;


//TODO: consider changing the naming from Set/Get to (for example) Expose/GetLocal, since these
// aren't the accessors and mutators in the traditional sense.
//--------------------------------------------------------------------------------------------------
/**
 * Set the local list of pools to the list in the mem module.
 */
//--------------------------------------------------------------------------------------------------
void spy_SetListOfPools
(
    le_dls_List_t* listOfPoolsRef      ///< [IN] Ref to the list of pools.
)
{
    ListOfPoolsRef = listOfPoolsRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the local list of pools to the list in the mem module.
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t* spy_GetListOfPools
(
    void
)
{
    return ListOfPoolsRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the local list of thread objects to the list in the thread module.
 */
//--------------------------------------------------------------------------------------------------
void spy_SetListOfThreadObj
(
    le_dls_List_t* listOfThreadObjsRef      ///< [IN] Ref to the list of thread objs.
)
{
    ListOfThreadObjsRef = listOfThreadObjsRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the local list of thread objects to the list in the thread module.
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t* spy_GetListOfThreadObj
(
    void
)
{
    return ListOfThreadObjsRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the local ref to the pools list change counter to the counter in the mem module.
 */
//--------------------------------------------------------------------------------------------------
void spy_SetListOfPoolsChgCntRef
(
    size_t** listOfPoolsChgCntRefRef ///< [IN] Ref to the list change counter for mem pools.
)
{
    ListOfPoolsChgCntRefRef = listOfPoolsChgCntRefRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the local ref to the pools list change counter to the counter in the mem module.
 */
//--------------------------------------------------------------------------------------------------
size_t** spy_GetListOfPoolsChgCntRef
(
    void
)
{
    return ListOfPoolsChgCntRefRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the local ref to the thread obj list change counter to the counter in the thread module.
 */
//--------------------------------------------------------------------------------------------------
void spy_SetListOfThreadObjsChgCntRef
(
    size_t** listOfThreadObjsChgCntRefRef ///< [IN] Ref to the list change counter for thread objs.
)
{
    ListOfThreadObjsChgCntRefRef = listOfThreadObjsChgCntRefRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the local ref to the thread obj list change counter to the counter in the thread module.
 */
//--------------------------------------------------------------------------------------------------
size_t** spy_GetListOfThreadObjsChgCntRef
(
    void
)
{
    return ListOfThreadObjsChgCntRefRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the local ref to the timer list change counter to the counter in the timer module.
 */
//--------------------------------------------------------------------------------------------------
void spy_SetListOfTimersChgCntRef
(
    size_t** listOfTimersChgCntRefRef     ///< [IN] Ref to the list change counter for timers.
)
{
    ListOfTimersChgCntRefRef = listOfTimersChgCntRefRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the local ref to the timer list change counter to the counter in the timer module.
 */
//--------------------------------------------------------------------------------------------------
size_t** spy_GetListOfTimersChgCntRef
(
    void
)
{
    return ListOfTimersChgCntRefRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the local ref to the mutex list change counter to the counter in the mutex module.
 */
//--------------------------------------------------------------------------------------------------
void spy_SetListOfMutexesChgCntRef
(
    size_t** listOfMutexesChgCntRefRef     ///< [IN] Ref to the list change counter for mutexes.
)
{
    ListOfMutexesChgCntRefRef = listOfMutexesChgCntRefRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the local ref to the mutex list change counter to the counter in the mutex module.
 */
//--------------------------------------------------------------------------------------------------
size_t** spy_GetListOfMutexesChgCntRef
(
    void
)
{
    return ListOfMutexesChgCntRefRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the local ref to the semaphore change counter to the counter in the semaphore module.
 */
//--------------------------------------------------------------------------------------------------
void spy_SetListOfSemaphoresChgCntRef
(
    size_t** listOfSemaphoresChgCntRefRef ///< [IN] Ref to the change counter for the semaphore.
)
{
    ListOfSemaphoresChgCntRefRef = listOfSemaphoresChgCntRefRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the local ref to the semaphore change counter to the counter in the semaphore module.
 */
//--------------------------------------------------------------------------------------------------
size_t** spy_GetListOfSemaphoresChgCntRef
(
    void
)
{
    return ListOfSemaphoresChgCntRefRef;
}
