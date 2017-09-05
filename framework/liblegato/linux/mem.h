//--------------------------------------------------------------------------------------------------
/** @file mem.h
 *
 * Legato memory management module's inter-module include file.
 *
 * This file exposes interfaces that are for use by other modules inside the framework
 * implementation, but must not be used outside of the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#ifndef MEM_INCLUDE_GUARD
#define MEM_INCLUDE_GUARD

#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * Definition of a memory pool.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mem_Pool
{
    le_dls_Link_t poolLink;             ///< This pool's link in the list of memory pools.
    struct le_mem_Pool* superPoolPtr;   ///< A pointer to our super pool if we are a sub-pool. NULL
                                        ///  if we are not a sub-pool.
    #ifndef LE_MEM_VALGRIND
        le_sls_List_t freeList;         ///< List of free memory blocks.
    #endif

    size_t userDataSize;                ///< Size of the object requested by the client in bytes.
    size_t blockSize;                   ///< Number of bytes in a block, including all overhead.
    uint64_t numAllocations;            ///< Total number of times an object has been allocated
                                        ///  from this pool.
    size_t numOverflows;                ///< Number of times le_mem_ForceAlloc() had to expand pool.
    size_t totalBlocks;                 ///< Total number of blocks in this pool including free
                                        ///  and allocated blocks.
    size_t numBlocksInUse;              ///< Number of currently allocated blocks.
    size_t maxNumBlocksUsed;            ///< Maximum number of allocated blocks at any one time.
    size_t numBlocksToForce;            ///< Number of blocks that is added when Force Alloc
                                        ///  expands the pool.
    #ifdef LE_MEM_TRACE
        le_log_TraceRef_t memTrace;     ///< If tracing is enabled, keeps track of a trace object
                                        ///  for this pool.
    #endif

    le_mem_Destructor_t destructor;     ///< The destructor for objects in this pool.
    char name[LIMIT_MAX_MEM_POOL_NAME_BYTES]; ///< Name of the pool.
}
MemPool_t;


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
 * Exposing the memory pool list; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t* mem_GetPoolList
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the memory pool list change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t** mem_GetPoolListChgCntRef
(
    void
);


#endif  // MEM_INCLUDE_GUARD
