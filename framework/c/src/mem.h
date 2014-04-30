//--------------------------------------------------------------------------------------------------
/** @file mem.h
 *
 * Legato memory mManagement module's inter-module include file.
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
 * The maximum size of the pool name.
 */
//--------------------------------------------------------------------------------------------------
#define MEM_MAX_POOL_NAME_BYTES         32


//--------------------------------------------------------------------------------------------------
/**
 * Definition of a memory pool.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mem_Pool
{
    le_dls_Link_t poolLink;             // This pool's link in the list of memory pools.
    struct le_mem_Pool* superPoolPtr;   // A pointer to our super pool if we are a sub-pool. NULL
                                        // if we are not a sub-pool.
    le_sls_List_t freeList;             // The list of free memory blocks.
    size_t userDataSize;                // The size of the object requested by the client in bytes.
    size_t totalBlockSize;              // The number of bytes in a block, including all overhead.
    uint64_t numAllocations;            // The total number of times an object has been allocated
                                        // from this pool.
    size_t numOverflows;                // The number of times le_mem_ForceAlloc() had to expand the
                                        // pool.
    size_t totalBlocks;                 // The total number of blocks in this pool including free
                                        // and allocated blocks.
    size_t numBlocksInUse;              // Number of currently allocated blocks.
    size_t maxNumBlocksUsed;            // Maximum number of allocated blocks at any one time.
    size_t numBlocksToForce;            // The number of blocks that is added when Force Alloc
                                        // expands the pool.
    le_mem_Destructor_t destructor;     // The destructor for objects in this pool.
    char name[MEM_MAX_POOL_NAME_BYTES]; // The name of the pool.
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
 * Returns a pointer to the list of pools.
 *
 * @note
 *      On failure, the process exits.
 */
//--------------------------------------------------------------------------------------------------
void* mem_GetListOfPools
(
    void
);

#endif  // MEM_INCLUDE_GUARD
