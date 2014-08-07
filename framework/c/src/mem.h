//--------------------------------------------------------------------------------------------------
/** @file mem.h
 *
 * Legato memory management module's inter-module include file.
 *
 * This file exposes interfaces that are for use by other modules inside the framework
 * implementation, but must not be used outside of the framework implementation.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 *
 */
#ifndef MEM_INCLUDE_GUARD
#define MEM_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the memory pool system.  This function must be called before any other memory pool
 * functions are called.
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
void mem_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Objects of this type are used to refer to a list of memory pools and can be used to iterate over
 * the list of available memory pools in a remote process.
 */
//--------------------------------------------------------------------------------------------------
typedef struct mem_iter_t* mem_Iter_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Creates an iterator that can be used to iterate over the list of available memory pools for a
 * specific process.
 *
 * @note
 *      The specified pid must be greater than zero.
 *
 *      The calling process must be root or have appropriate capabilities for this function and all
 *      subsequent operations on the iterator to succeed.
 *
 *      If NULL is returned the errorPtr will be set appropriately.  Possible values are:
 *      LE_NOT_POSSIBLE if the specified process is not a Legato process.
 *      LE_FAULT if there was some other error.
 *
 *      errorPtr can be NULL if the error code is not needed.
 *
 * @return
 *      An iterator to the list of memory pools for the specified process.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
mem_Iter_Ref_t mem_iter_Create
(
    pid_t pid,                  ///< [IN] The process to get the iterator for.
    le_result_t *errorPtr       ///< [OUT] Error code.  See comment block for more details.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next memory pool from the specified iterator.  The first time this function is called
 * after mem_iter_Create() is called the first memory pool in the list is returned.  The second
 * time this function is called the second memory pool is returned and so on.
 *
 * @warning
 *      The memory pool returned by this function belongs to the remote process.  Do not attempt to
 *      expand the pool or allocate objects from the pool, doing so will lead to memory leaks in
 *      the calling process.
 *
 * @return
 *      A memory pool from the iterator's list of memory pools.
 *      NULL if there are no more memory pools in the list.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t mem_iter_GetNextPool
(
    mem_Iter_Ref_t iterator     ///< [IN] The iterator to get the next mem pool from.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the iterator.
 */
//--------------------------------------------------------------------------------------------------
void mem_iter_Delete
(
    mem_Iter_Ref_t iterator     ///< [IN] The iterator to delete.
);


#endif  // MEM_INCLUDE_GUARD
