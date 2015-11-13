//--------------------------------------------------------------------------------------------------
/** @file spy.h
 *
 * Legato Inspection tool's spy - inter-module include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
#ifndef SPY_INCLUDE_GUARD
#define SPY_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Set the local list of pools to the list in the mem module.
 */
//--------------------------------------------------------------------------------------------------
void spy_SetListOfPools
(
    le_dls_List_t* listOfPoolsRef      ///< [IN] Ref to the list of pools.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the local list of pools to the list in the mem module.
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t* spy_GetListOfPools
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the local list of thread objects to the list in the thread module.
 */
//--------------------------------------------------------------------------------------------------
void spy_SetListOfThreadObj
(
    le_dls_List_t* listOfThreadObjsRef      ///< [IN] Ref to the list of thread objs.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the local list of thread objects to the list in the thread module.
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t* spy_GetListOfThreadObj
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the local ref to the pools list change counter to the counter in the mem module.
 */
//--------------------------------------------------------------------------------------------------
void spy_SetListOfPoolsChgCntRef
(
    size_t** listOfPoolsChgCntRefRef ///< [IN] Ref to the list change counter for mem pools.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the local ref to the pools list change counter to the counter in the mem module.
 */
//--------------------------------------------------------------------------------------------------
size_t** spy_GetListOfPoolsChgCntRef
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the local ref to the thread obj list change counter to the counter in the thread module.
 */
//--------------------------------------------------------------------------------------------------
void spy_SetListOfThreadObjsChgCntRef
(
    size_t** listOfThreadObjsChgCntRefRef ///< [IN] Ref to the list change counter for thread objs.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the local ref to the thread obj list change counter to the counter in the thread module.
 */
//--------------------------------------------------------------------------------------------------
size_t** spy_GetListOfThreadObjsChgCntRef
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the local ref to the timer list change counter to the counter in the timer module.
 */
//--------------------------------------------------------------------------------------------------
void spy_SetListOfTimersChgCntRef
(
    size_t** listOfTimersChgCntRefRef     ///< [IN] Ref to the list change counter for timers.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the local ref to the timer list change counter to the counter in the timer module.
 */
//--------------------------------------------------------------------------------------------------
size_t** spy_GetListOfTimersChgCntRef
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the local ref to the mutex list change counter to the counter in the mutex module.
 */
//--------------------------------------------------------------------------------------------------
void spy_SetListOfMutexesChgCntRef
(
    size_t** listOfMutexesChgCntRefRef     ///< [IN] Ref to the list change counter for mutexes.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the local ref to the mutex list change counter to the counter in the mutex module.
 */
//--------------------------------------------------------------------------------------------------
size_t** spy_GetListOfMutexesChgCntRef
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the local ref to the semaphore change counter to the counter in the semaphore module.
 */
//--------------------------------------------------------------------------------------------------
void spy_SetListOfSemaphoresChgCntRef
(
    size_t** listOfSemaphoresChgCntRefRef ///< [IN] Ref to the change counter for the semaphore.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the local ref to the semaphore change counter to the counter in the semaphore module.
 */
//--------------------------------------------------------------------------------------------------
size_t** spy_GetListOfSemaphoresChgCntRef
(
    void
);


#endif  // SPY_INCLUDE_GUARD
