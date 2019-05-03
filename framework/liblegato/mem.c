//--------------------------------------------------------------------------------------------------
/** @file mem.c
 *
 * This module maintains a local list of memory pools that contain all memory pools created in this
 * process.  Each memory pool contains a collection of fixed-size memory blocks, each of which
 * contains a single user object, plus some overhead.  Since each memory block contains one user
 * object, the number of blocks and objects in a memory pool are always the same.
 *
 * Memory for the memory blocks (including the user object) is allocated from system
 * memory when a memory pool is expanded.  Memory blocks are never released back to system memory.
 * Instead, when they are "free", they are kept on their pool's "free list".  The free list is
 * O(1) for both insertion and removal.  It is treated as a stack, in that blocks are popped from
 * the head of the free list when they are allocated and pushed back onto the head of the free
 * list when they are deallocated.  The hope is that this will speed things up by utilizing the
 * cache better when there are a lot of allocations interleaved with releases.
 *
 * Sub-pools behave exactly like memory pools except in the way that they are created, expanded and
 * deleted.
 *
 * A sub-pool must be created using the le_mem_CreateSubPool function rather than the
 * le_mem_CreatePool function.  When a sub-pool is created the memory for the sub-pool is allocated
 * from the local memory pool of sub-pools.  The created sub-pool is then added to the local list
 * of pools.
 *
 * The super-pool for a sub-pool must be one of the memory pools created with le_mem_CreatePool.  In
 * other words sub-pools of sub-pools are not allowed.
 *
 * Unlike a memory pool, which cannot be deleted, a sub-pool can be deleted.  When a sub-pool is
 * deleted the sub-pool's blocks are released back into the super-pool.  However, it is an error to
 * delete a sub-pool while there are still blocks allocated from it.  The sub-pool itself is then
 * removed from the list of pools and released back into the pool of sub-pools.
 *
 * GUARD BANDS
 * ===========
 *
 * A debugging feature can be enabled at build time by enabling the @ref USE_GUARD_BAND KConfig
 * option.  This inserts chunks of memory into each memory block both before and after the user
 * object part.  These chunks of memory, called "guard bands", are filled with a special pattern
 * that is unlikely to occur in normal data.  Whenever a block is allocated or released, the guard
 * bands are checked for corruption and any corruption is reported.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#include "legato.h"
#include "mem.h"

#define GUARD_WORD ((uint32_t)0xDEADBEEF)
#define GUARD_BAND_SIZE (sizeof(GUARD_WORD) * LE_CONFIG_NUM_GUARD_BAND_WORDS)


/// The maximum total pool name size, including the component prefix, which is a component
/// name plus a '.' separator ("myComp.myPool") and the null terminator.
#define MAX_POOL_NAME_BYTES (LIMIT_MAX_COMPONENT_NAME_LEN + 1 + LIMIT_MAX_MEM_POOL_NAME_BYTES)

/// The default number of Sub Pool objects in the Sub Pools Pool.
/// @todo Make this configurable.
#define DEFAULT_SUB_POOLS_POOL_SIZE     8

//--------------------------------------------------------------------------------------------------
/**
 * Insert a string name variable if configured or a placeholder string if not.
 *
 *  @param  nameVar Name variable to insert.
 *
 *  @return Name variable or a placeholder string depending on configuration.
 **/
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
#   define  MEMPOOL_NAME(var) (var)
#else
#   define  MEMPOOL_NAME(var) "<omitted>"
#endif

//--------------------------------------------------------------------------------------------------
// Create definitions for inlineable functions
//
// See le_mem.h for bodies & documentation
//--------------------------------------------------------------------------------------------------
#if !LE_CONFIG_EVENT_NAMES_ENABLED
LE_DEFINE_INLINE le_mem_PoolRef_t le_mem_CreatePool
(
    const char* name,           ///< [IN] Name of the pool inside the component.
    size_t      objSize ///< [IN] Size of the individual objects to be allocated from this pool
                        /// (in bytes), e.g., sizeof(MyObject_t).
);

LE_DEFINE_INLINE le_mem_PoolRef_t le_mem_FindPool
(
    const char* name            ///< [IN] Name of the pool inside the component.
);

LE_DEFINE_INLINE le_mem_PoolRef_t le_mem_CreateSubPool
(
    le_mem_PoolRef_t    superPool,      ///< [IN] Super-pool.
    const char*         name,           ///< [IN] Name of the sub-pool (will be copied into the
                                        ///   sub-pool).
    size_t              numObjects      ///< [IN] Number of objects to take from the super-pool.
);

LE_DEFINE_INLINE le_mem_PoolRef_t le_mem_CreateReducedPool
(
    le_mem_PoolRef_t    superPool,      ///< [IN] Super-pool.
    const char*         name,           ///< [IN] Name of the sub-pool (will be copied into the
                                        ///   sub-pool).
    size_t              numObjects,     ///< [IN] Minimum number of objects in the subpool
                                        ///< by default.
    size_t              objSize         ///< [IN] Minimum size of objects in the subpool.
);
#endif
//--------------------------------------------------------------------------------------------------
/**
 * The default number of blocks to expand by when the le_mem_ForceAlloc expands the memory pool.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_NUM_BLOCKS_TO_FORCE     1


#if LE_CONFIG_MEM_TRACE
#   undef le_mem_TryAlloc
#   undef le_mem_AssertAlloc
#   undef le_mem_ForceAlloc
#   undef le_mem_Release
#   undef le_mem_AddRef

#   define le_mem_TryAlloc      _le_mem_TryAlloc
#   define le_mem_AssertAlloc   _le_mem_AssertAlloc
#   define le_mem_ForceAlloc    _le_mem_ForceAlloc
#   define le_mem_Release       _le_mem_Release
#   define le_mem_AddRef        _le_mem_AddRef
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Definition of a memory block.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_mem_Pool_t* poolPtr; ///< A pointer to the pool (or sub-pool) that this block belongs to.

    size_t refCount;        ///< The number of external references to this memory block's
                            ///<     user object. (0 = free)
    union
    {
        le_sls_Link_t link;
        uint8_t item;
    } data[];
}
MemBlock_t;


//--------------------------------------------------------------------------------------------------
/**
 * Local list of all memory pools created with le_mem_CreatePool and le_mem_CreateSubPool
 * within this process.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t PoolList = LE_DLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * A counter that increments every time a change is made to PoolList.
 */
//--------------------------------------------------------------------------------------------------
static size_t PoolListChangeCount = 0;
static size_t* PoolListChangeCountRef = &PoolListChangeCount;


//--------------------------------------------------------------------------------------------------
/**
 * Static memory pool for sub-pools.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(SubPools, LE_CONFIG_MAX_SUB_POOLS_POOL_SIZE, sizeof(le_mem_Pool_t));


//--------------------------------------------------------------------------------------------------
/**
 * Local memory pool that is used for allocating sub-pools.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SubPoolsPool;


//--------------------------------------------------------------------------------------------------
/**
 * Pthreads fast mutex used to protect data structures in this module from multithreading races.
 */
//--------------------------------------------------------------------------------------------------
static pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the memory pool list; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t* mem_GetPoolList
(
    void
)
{
    return (&PoolList);
}


//--------------------------------------------------------------------------------------------------
/**
 * Exposing the memory pool list change counter; mainly for the Inspect tool.
 */
//--------------------------------------------------------------------------------------------------
size_t** mem_GetPoolListChgCntRef
(
    void
)
{
    return (&PoolListChangeCountRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Locks the mutex.
 */
//--------------------------------------------------------------------------------------------------
#if !LE_CONFIG_RTOS
static inline
#endif
void mem_Lock
(
    void
)
{
    LE_ASSERT(pthread_mutex_lock(&Mutex) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Unlocks the mutex.
 */
//--------------------------------------------------------------------------------------------------
#if !LE_CONFIG_RTOS
static inline
#endif
void mem_Unlock
(
    void
)
{
    LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0);
}


#if LE_CONFIG_USE_GUARD_BAND

    //----------------------------------------------------------------------------------------------
    /**
     * Initializes the guard bands in a memory block's data payload section.
     */
    //----------------------------------------------------------------------------------------------
    static void InitGuardBands
    (
        MemBlock_t* blockHeaderPtr  // Pointer to the per-block overhead area of the memory block.
    )
    {
        int i;

        // There's a guard band at the start of the data section.
        uint32_t* guardBandWordPtr = (uint32_t*)(blockHeaderPtr->data);
        for (i = 0; i < LE_CONFIG_NUM_GUARD_BAND_WORDS; i++, guardBandWordPtr++)
        {
            *guardBandWordPtr = GUARD_WORD;
        }

        // There's another guard band at the end of the data section.
        guardBandWordPtr = (uint32_t*)(   ((uint8_t*)blockHeaderPtr)
                                        + blockHeaderPtr->poolPtr->blockSize
                                        - GUARD_BAND_SIZE );
        for (i = 0; i < LE_CONFIG_NUM_GUARD_BAND_WORDS; i++, guardBandWordPtr++)
        {
            *guardBandWordPtr = GUARD_WORD;
        }
    }

    //----------------------------------------------------------------------------------------------
    /**
     * Checks the integrity of the guard bands in a memory block's data payload section.
     */
    //----------------------------------------------------------------------------------------------
    static void CheckGuardBands
    (
        MemBlock_t* blockHeaderPtr  // Pointer to the per-block overhead area of the memory block.
    )
    {
        int i;

        // There's a guard band at the start of the data section.
        uint32_t* guardBandWordPtr = (uint32_t*)(blockHeaderPtr->data);
        for (i = 0; i < LE_CONFIG_NUM_GUARD_BAND_WORDS; i++, guardBandWordPtr++)
        {
            if (*guardBandWordPtr != GUARD_WORD)
            {
                LE_EMERG("Memory corruption detected at address %p before object allocated"
                                                                                " from pool '%s'.",
                         guardBandWordPtr,
                         MEMPOOL_NAME(blockHeaderPtr->poolPtr->name));
                LE_FATAL("Guard band value should have been %" PRIu32 ", but was found to be "
                         "%" PRIu32 ".",
                         GUARD_WORD,
                         *guardBandWordPtr);
            }
        }

        // There's another guard band at the end of the data section.
        guardBandWordPtr = (uint32_t*)(   ((uint8_t*)blockHeaderPtr)
                                        + blockHeaderPtr->poolPtr->blockSize
                                        - GUARD_BAND_SIZE);
        for (i = 0; i < LE_CONFIG_NUM_GUARD_BAND_WORDS; i++, guardBandWordPtr++)
        {
            if (*guardBandWordPtr != GUARD_WORD)
            {
                LE_EMERG("Memory corruption detected at address %p at end of object allocated"
                                                                                " from pool '%s'.",
                         guardBandWordPtr,
                         MEMPOOL_NAME(blockHeaderPtr->poolPtr->name));
                LE_FATAL("Guard band value should have been %" PRIu32 ", but was found to be "
                         "%" PRIu32 ".",
                         GUARD_WORD,
                         *guardBandWordPtr);
            }
        }
    }

#endif

//---------------------------------------------------------------------------------------------------
/**
 * Get the size of a block required for storing an object, given the size of the object.
 *
 * @return The block size
 */
//---------------------------------------------------------------------------------------------------
size_t CalcBlockSize
(
    size_t objSize    ///< [IN] The size of the object
)
{
    size_t blockSize = 0;

    if (objSize < sizeof(le_sls_Link_t))
    {
        blockSize = sizeof(MemBlock_t) + sizeof(le_sls_Link_t);
    }
    else
    {
        blockSize = sizeof(MemBlock_t) + objSize;
    }

    // Add guard bands around the user data in every block.
    blockSize += (GUARD_BAND_SIZE * 2);

    // Round up the block size to the nearest multiple of the processor word size.
    size_t remainder = blockSize % sizeof(void*);
    if (remainder != 0)
    {
        blockSize += (sizeof(void*) - remainder);
    }

    return blockSize;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initializes a memory pool.
 *
 * @warning Called without the mutex locked.
 */
//--------------------------------------------------------------------------------------------------
static void InitPool
(
    le_mem_PoolRef_t    pool,           ///< [IN] The pool to initialize.
#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
    const char*         componentName,  ///< [IN] Name of the component.
    const char*         name,           ///< [IN] Name of the pool inside the component.
#endif /* end LE_CONFIG_MEM_POOL_NAMES_ENABLED */
    size_t              objSize         ///< [IN] The size of the individual objects to be allocated
                                        ///       from this pool in bytes.
)
{
#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
    // Construct the component-scoped pool name.
    size_t nameSize = snprintf(pool->name, sizeof(pool->name), "%s.%s", componentName, name);
    if (nameSize >= sizeof(pool->name))
    {
        LE_DEBUG("Memory pool name '%s.%s' is truncated to '%s'", componentName, name, pool->name);
    }
#endif /* end LE_CONFIG_MEM_POOL_NAMES_ENABLED */

    // Compute the total block size.
    size_t blockSize = CalcBlockSize(objSize);

    // When initializing a static block, do not zero members as these are already
    // zeroed by library initialization
    pool->userDataSize = objSize;
    pool->blockSize = blockSize;
    pool->numBlocksToForce = DEFAULT_NUM_BLOCKS_TO_FORCE;

    pool->poolLink = LE_DLS_LINK_INIT;

#if LE_CONFIG_MEM_TRACE
    pool->memTrace = NULL;

    if (LE_LOG_SESSION != NULL)
    {
        pool->memTrace = le_log_GetTraceRef(pool->name);
        LE_DEBUG("Tracing enabled for pool '%s'.", pool->name);
    }
#endif
}

#if LE_CONFIG_MEM_POOLS
//--------------------------------------------------------------------------------------------------
/**
 * Add at least the specified number of blocks to the destination pool by removing from the
 * source pool.
 *
 * @note
 *      - Does not update the total number of blocks in either pool.
 *      - Assumes that the mutex is locked.
 *      - If the source pool has different sized blocks than the destination pool, the number of
 *        blocks removed from the source pool may not match the number added to the destination.
 *
 * @return The number of blocks removed from the source pool.
 */
//--------------------------------------------------------------------------------------------------
static size_t MoveBlocks
(
    le_mem_PoolRef_t    destPool,   ///< [IN] The pool to move the blocks to.
    le_mem_PoolRef_t    srcPool,    ///< [IN] The pool to get the blocks from.
    size_t              numBlocks   ///< [IN] The maximum number of blocks to move to destination pool.
)
{
    LE_FATAL_IF(destPool->blockSize > srcPool->blockSize,
                "Cannot move blocks from a smaller pool to a larger pool with this function");

    size_t i = 0, j = 0, removedCount = 0;
    while (i < numBlocks)
    {
        // Get the first block to move.
        LE_DEBUG("Getting next block from source pool");

        le_sls_Link_t* blockLinkPtr = le_sls_Pop(&(srcPool->freeList));
        ++removedCount;

        if (blockLinkPtr == NULL)
        {
            LE_FATAL("Asked to move %" PRIuS " blocks from pool '%s' to pool '%s', "
                     "but only %" PRIuS " were available.",
                     numBlocks,
                     MEMPOOL_NAME(srcPool->name),
                     MEMPOOL_NAME(destPool->name),
                     i);
        }

        for (j = destPool->blockSize; j <= srcPool->blockSize; j += destPool->blockSize, ++i)
        {
            // Add the block to the destination pool.
            *blockLinkPtr = LE_SLS_LINK_INIT;
            le_sls_Stack(&(destPool->freeList), blockLinkPtr);

            LE_DEBUG("Moved block %"PRIuS"/%"PRIuS, i+1, numBlocks);

            // Update the blocks parent pool.
            MemBlock_t* blockPtr = CONTAINER_OF(blockLinkPtr, MemBlock_t, data[0].link);
            blockPtr->poolPtr = destPool;
            blockPtr->refCount = 0;

            blockLinkPtr = (le_sls_Link_t*)((char*)blockLinkPtr + destPool->blockSize);
        }
    }

    return removedCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Compare two list items to sort in order of ascending address.
 *
 * @return true if aPtr is before bPtr in the list
 */
//--------------------------------------------------------------------------------------------------
static bool AddrCompare
(
    le_sls_Link_t *aPtr,    ///< [IN] Pointer to an element in the list
    le_sls_Link_t *bPtr     ///< [IN] Pointer to another element in the list
)
{
    // Order elements by their addresses
    return aPtr < bPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Moves all blocks from the source pool to the destination pool.
 *
 * @note
 *      Does not update the total number of blocks for either pool.
 *      Assumes that the mutex is locked.  Assumes destPool is a super-pool of srcPool.
 *
 * @return Number of blocks added to the destination pool.
 */
//--------------------------------------------------------------------------------------------------
static size_t MoveAllBlocks
(
    le_mem_PoolRef_t    destPool,   ///< [IN] The pool to move the blocks to.
    le_mem_PoolRef_t    srcPool     ///< [IN] The pool to get the blocks from.
)
{
    size_t blockMoveCount = 0;

    LE_FATAL_IF(srcPool->numBlocksInUse != 0,
                "Cannot move all blocks from source pool as some are still in use");
    LE_FATAL_IF(destPool->blockSize < srcPool->blockSize,
                "Cannot move blocks from a larger pool to a smaller pool with this function");

    // If destination blocks are larger than source blocks, recombine into larger blocks.
    if (destPool->blockSize > srcPool->blockSize)
    {
        le_sls_Sort(&srcPool->freeList, AddrCompare);
        while (1)
        {
            size_t blockSize;
            le_sls_Link_t *blockLinkPtr = le_sls_Pop(&srcPool->freeList);
            if (!blockLinkPtr)
            {
                break;
            }

            for (blockSize = srcPool->blockSize;
                 blockSize <= destPool->blockSize - srcPool->blockSize;
                 blockSize += srcPool->blockSize)
            {
                // Pop next block from source free list to combine into new block being built-up.
                // Check the combine block is adjacent super-block being built-up
                le_sls_Link_t *combineBlock = le_sls_Pop(&srcPool->freeList);
                LE_DEBUG("Combine with %p", combineBlock);
                LE_ASSERT((char*)blockLinkPtr + blockSize == (char*)combineBlock);
            }

            if (destPool->blockSize - srcPool->blockSize < blockSize &&
                blockSize <= destPool->blockSize)
            {
                // Add the block to the destination pool.
                le_sls_Stack(&(destPool->freeList), blockLinkPtr);
                blockMoveCount++;
            }
            else
            {
                LE_ERROR("Could not re-assemble block at %p"
                         " (expected size ~%"PRIuS", found %"PRIuS"); block lost",
                         blockLinkPtr, destPool->blockSize, blockSize);
            }

            // Update the blocks parent pool.
            MemBlock_t* blockPtr = CONTAINER_OF(blockLinkPtr, MemBlock_t, data[0].link);
            blockPtr->poolPtr = destPool;
            blockPtr->refCount = 0;
        }

        return blockMoveCount;
    }
    else
    {
        // Same size, just move them all.
        return MoveBlocks(destPool, srcPool, srcPool->totalBlocks);
    }
}

#endif /* end LE_CONFIG_MEM_POOLS */

//--------------------------------------------------------------------------------------------------
/**
 * Initialize a new pool block.
 */
//--------------------------------------------------------------------------------------------------
static void InitBlock
(
    le_mem_PoolRef_t pool,       ///< [IN] The pool the new block belongs to.
    MemBlock_t* newBlockPtr      ///< [IN] The block being initialized.
)
{
    // Initialize the block.
#if LE_CONFIG_MEM_POOLS
    // Add the block to the pool's free list.
    newBlockPtr->data[0].link = LE_SLS_LINK_INIT;
    le_sls_Stack(&(pool->freeList), &(newBlockPtr->data[0].link));
#endif

    newBlockPtr->refCount = 0;
    newBlockPtr->poolPtr = pool;
}


#if LE_CONFIG_MEM_POOLS
    //----------------------------------------------------------------------------------------------
    /**
     * Creates blocks and adds them to the pool.
     *
     * @note
     *      Updates the pools total number of blocks.
     *
     * @note
     *      Assumes that the mutex is locked.
     */
    //----------------------------------------------------------------------------------------------
    static void AddBlocks
    (
        le_mem_PoolRef_t    pool,       ///< [IN] The pool to be expanded.
        size_t              numBlocks   ///< [IN] The number of blocks to add to the pool.
    )
    {
        size_t i;
        size_t blockSize = pool->blockSize;
        size_t mallocSize = numBlocks * blockSize;

        // Allocate the chunk.
        MemBlock_t* newBlockPtr = malloc(mallocSize);

        LE_ASSERT(newBlockPtr);

        for (i = 0; i < numBlocks; i++)
        {
            InitBlock(pool, newBlockPtr);
            newBlockPtr = (MemBlock_t*)(((uint8_t*)newBlockPtr) + blockSize);
        }

        // Update the pool.
        pool->totalBlocks += numBlocks;
    }
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Log an error message if there is another pool with the same name as a given pool.
 */
//--------------------------------------------------------------------------------------------------
static void VerifyUniquenessOfName
(
    le_mem_PoolRef_t newPool
)
//--------------------------------------------------------------------------------------------------
{
#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
    le_dls_Link_t* poolLinkPtr = le_dls_Peek(&PoolList);

    while (poolLinkPtr)
    {
        le_mem_Pool_t* memPoolPtr = CONTAINER_OF(poolLinkPtr, le_mem_Pool_t, poolLink);

        if ((strcmp(newPool->name, memPoolPtr->name) == 0) && (newPool != memPoolPtr))
        {
            LE_WARN("Multiple memory pools share the same name '%s'."
                    " This will become illegal in future releases.\n", memPoolPtr->name);
            break;
        }

        poolLinkPtr = le_dls_PeekNext(&PoolList, poolLinkPtr);
    }
#endif /* end LE_CONFIG_MEM_POOL_NAMES_ENABLED */
}


//--------------------------------------------------------------------------------------------------
/**
 * Sub-pool destructor.
 */
//--------------------------------------------------------------------------------------------------
void SubPoolDestructor
(
    void* subPoolPtr            ///< [IN] Sub-pool being destroyed
)
{
    le_mem_PoolRef_t subPool = subPoolPtr;

    mem_Lock();

    LE_FATAL_IF(subPool->numBlocksInUse != 0,
                "Subpool '%s' deleted while %" PRIuS " blocks remain allocated.",
                MEMPOOL_NAME(subPool->name),
                subPool->numBlocksInUse);

#if LE_CONFIG_MEM_POOLS
    // Move the blocks from the subPool back to the superpool.
    // And update the superPool's block use count.
    size_t blocksFreed = MoveAllBlocks(subPool->superPoolPtr, subPool);
    LE_FATAL_IF(blocksFreed > subPool->superPoolPtr->numBlocksInUse,
                "More blocks returned to pool (%" PRIuS ") than present in pool (%" PRIuS ")",
                blocksFreed, subPool->superPoolPtr->numBlocksInUse);
    subPool->superPoolPtr->numBlocksInUse -= blocksFreed;
#endif

    // Remove the sub-pool from the list of sub-pools.
    PoolListChangeCount++;
    le_dls_Remove(&PoolList, &(subPool->poolLink));

    mem_Unlock();
}


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
)
{
    // NOTE: No need to lock the mutex because this function should be called when there is still
    //       only one thread running.

    // Create a memory for all sub-pools.
    SubPoolsPool = le_mem_InitStaticPool(SubPools,
                                         LE_CONFIG_MAX_SUB_POOLS_POOL_SIZE,
                                         sizeof(le_mem_Pool_t));
    le_mem_SetDestructor(SubPoolsPool, SubPoolDestructor);
}


//----------------------------------------------------------------------------------------------
/**
 * Internal function used to retrieve a pool handle for a given pool block.
 */
//----------------------------------------------------------------------------------------------
le_mem_PoolRef_t _le_mem_GetBlockPool
(
    void*   objPtr  ///< [IN] Pointer to the object we're finding a pool for.
)
{
    MemBlock_t* blockPtr;

    // Get the block from the object pointer.
#if LE_CONFIG_USE_GUARD_BAND
    uint8_t* dataPtr = objPtr;
    dataPtr -= GUARD_BAND_SIZE;
    blockPtr = CONTAINER_OF(dataPtr, MemBlock_t, data);
#else
    blockPtr = CONTAINER_OF(objPtr, MemBlock_t, data);
#endif

#if LE_CONFIG_USE_GUARD_BAND
    CheckGuardBands(blockPtr);
#endif

    return blockPtr->poolPtr;
}

#if LE_CONFIG_MEM_TRACE
    //----------------------------------------------------------------------------------------------
    /**
     * Internal function used to call a memory allocation function and trace it's call site.
     */
    //----------------------------------------------------------------------------------------------
    void* _le_mem_AllocTracer
    (
        le_mem_PoolRef_t     pool,             ///< [IN] The pool activity we're tracing.
        _le_mem_AllocFunc_t  funcPtr,          ///< [IN] Pointer to the mem function in question.
        const char*          poolFunction,     ///< [IN] The pool function being called.
        const char*          file,             ///< [IN] The file the call came from.
        const char*          callingfunction,  ///< [IN] The function calling into the pool.
        size_t               line              ///< [IN] The line in the function where the call
                                               ///       occurred.
    )
    {
        void* blockPtr = funcPtr(pool);
        _le_mem_Trace(pool, file, callingfunction, line, poolFunction, blockPtr);

        return blockPtr;
    }


    //----------------------------------------------------------------------------------------------
    /**
     * Internal function used to trace memory pool activity.
     */
    //----------------------------------------------------------------------------------------------
    void _le_mem_Trace
    (
        le_mem_PoolRef_t    pool,            ///< [IN] The pool activity we're tracing.
        const char*         file,            ///< [IN] The file the call came from.
        const char*         callingfunction, ///< [IN] The function calling into the pool.
        size_t              line,            ///< [IN] The line in the function where the call
                                             ///  occurred.
        const char*         poolFunction,    ///< [IN] The pool function being called.
        void*               blockPtr         ///< [IN] Block allocated/freed.
    )
    {
        le_log_TraceRef_t trace = pool->memTrace;

        if (trace && le_log_IsTraceEnabled(trace))
        {
#   if LE_CONFIG_MEM_POOL_NAMES_ENABLED
            char poolName[LIMIT_MAX_MEM_POOL_NAME_BYTES] = "";
            LE_ASSERT(le_mem_GetName(pool, poolName, sizeof(poolName)) == LE_OK);
#   endif /* LE_CONFIG_MEM_POOL_NAMES_ENABLED */

            _le_log_Send((le_log_Level_t)-1,
                         trace,
                         LE_LOG_SESSION,
                         le_path_GetBasenamePtr(file, "/"),
                         callingfunction,
                         line,
                         "%s: %s, %p",
                         MEMPOOL_NAME(poolName),
                         poolFunction,
                         blockPtr);
        }
    }
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Creates an empty memory pool.
 *
 * @return
 *      A reference to the memory pool object.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
le_mem_PoolRef_t _le_mem_CreatePool
(
    const char*     componentName,  ///< [IN] Name of the component.
    const char*     name,           ///< [IN] Name of the pool inside the component.
    size_t          objSize         ///< [IN] The size of the individual objects to be allocated
                                    /// from this pool (in bytes).  E.g., sizeof(MyObject_t).
)
#else
le_mem_PoolRef_t _le_mem_CreatePool
(
    size_t      objSize ///< [IN] Size of the individual objects to be allocated from this pool
                        /// (in bytes), e.g., sizeof(MyObject_t).
)
#endif
{
    le_mem_PoolRef_t newPool = calloc(1, sizeof(le_mem_Pool_t));

    // Crash if we can't create the memory pool.
    LE_ASSERT(newPool);

    // Initialize the memory pool.
#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
    InitPool(newPool, componentName, name, objSize);
#else
    InitPool(newPool, objSize);
#endif

    mem_Lock();

    // Generate an error if there are multiple pools with the same name.
    VerifyUniquenessOfName(newPool);

    // Add the new pool to the list of pools.
    PoolListChangeCount++;
    le_dls_Queue(&PoolList, &(newPool->poolLink));

    mem_Unlock();

    return newPool;
}


//--------------------------------------------------------------------------------------------------
/**
 * Internal function used to implement le_mem_InitStaticPool() with automatic component scoping
 * of pool names.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t _le_mem_InitStaticPool
(
    const char* componentName,  ///< [IN] Name of the component.
    const char* name,           ///< [IN] Name of the pool inside the component.
    size_t      numBlocks,      ///< [IN] Number of members in the pool by default
    size_t      objSize,        ///< [IN] Size of the individual objects to be allocated from this
                                ///<      pool (in bytes), e.g., sizeof(MyObject_t).
    le_mem_Pool_t* poolPtr,     ///< [IN] Pointer to pre-allocated pool header.
    void* poolDataPtr           ///< [IN] Pointer to pre-allocated pool data.
)
{
    // Initialize the memory pool.
#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
    InitPool(poolPtr, componentName, name, objSize);
#else
    InitPool(poolPtr, objSize);
#endif

    mem_Lock();

    // Generate an error if there are multiple pools with the same name.
    VerifyUniquenessOfName(poolPtr);

    // Add the new pool to the list of pools.
    PoolListChangeCount++;
    le_dls_Queue(&PoolList, &(poolPtr->poolLink));

    mem_Unlock();

#if LE_CONFIG_MEM_POOLS
    size_t blockSize = poolPtr->blockSize;

    size_t i;
    for (i = 0; i < numBlocks; i++)
    {
        InitBlock(poolPtr, poolDataPtr);
        poolDataPtr = (MemBlock_t*)(((uint8_t*)poolDataPtr) + blockSize);
    }

    // Update the pool.
    poolPtr->totalBlocks += numBlocks;
#endif

    return poolPtr;
}



//--------------------------------------------------------------------------------------------------
/**
 * Internal function to expand the size of a memory pool.  Assumes memory is already locked.
 *
 * @return  A reference to the memory pool object (the same value passed into it).
 *
 * @note    On failure, the process exits, so you don't have to worry about checking the returned
 *          reference for validity.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ExpandPool_NoLock
(
    le_mem_PoolRef_t    pool,       ///< [IN] The pool to be expanded.
    size_t              numObjects  ///< [IN] The number of objects to add to the pool.
)
{
#if LE_CONFIG_MEM_POOLS
    LE_ASSERT(pool);

    if (pool->superPoolPtr)
    {
        // This is a sub-pool so the memory blocks to create must come from the super-pool.
        // Check that there are enough blocks in the superpool.
        size_t superBlocksPerBlock = (pool->superPoolPtr->blockSize/pool->blockSize);
        ssize_t numBlocksToAdd = (numObjects + superBlocksPerBlock - 1)/superBlocksPerBlock
                                    - le_sls_NumLinks(&(pool->superPoolPtr->freeList));

        if (numBlocksToAdd > 0)
        {
            // Expand the super-pool.
            LE_DEBUG("Expanding super-pool by %" PRIuS " blocks", numBlocksToAdd);
            ExpandPool_NoLock(pool->superPoolPtr, numBlocksToAdd);

#   if LE_CONFIG_MEM_POOL_STATS
            // This counts as an overflow for the super-pool -- expect super pools to be
            // satisified within their current allocations
            pool->superPoolPtr->numOverflows += numBlocksToAdd;
#   endif /* end LE_CONFIG_MEM_POOL_STATS */
        }

        // Move the blocks from the superpool to our pool.
        size_t removedBlocks = MoveBlocks(pool, pool->superPoolPtr, numObjects);

        // Update the sub-pool total block count.
        pool->totalBlocks += removedBlocks * (pool->superPoolPtr->blockSize/pool->blockSize);

        // Update the super-pool's block use counts.
        pool->superPoolPtr->numBlocksInUse += removedBlocks;

#   if LE_CONFIG_MEM_POOL_STATS
        if (pool->superPoolPtr->numBlocksInUse > pool->superPoolPtr->maxNumBlocksUsed)
        {
            pool->superPoolPtr->maxNumBlocksUsed = pool->superPoolPtr->numBlocksInUse;
        }
#   endif /* end LE_CONFIG_MEM_POOL_STATS */
    }
    else
    {
        // This is not a sub-pool.
        AddBlocks(pool, numObjects);
    }
#endif /* end LE_CONFIG_MEM_POOLS */

    return pool;
}


//--------------------------------------------------------------------------------------------------
/**
 * Expands the size of a memory pool.
 *
 * @return  A reference to the memory pool object (the same value passed into it).
 *
 * @note    On failure, the process exits, so you don't have to worry about checking the returned
 *          reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t le_mem_ExpandPool
(
    le_mem_PoolRef_t    pool,       ///< [IN] The pool to be expanded.
    size_t              numObjects  ///< [IN] The number of objects to add to the pool.
)
{
#if LE_CONFIG_MEM_POOLS
    LE_ASSERT(pool);

    mem_Lock();

    pool = ExpandPool_NoLock(pool, numObjects);

    mem_Unlock();
#endif

    return pool;
}


//--------------------------------------------------------------------------------------------------
/**
 * Attempts to allocate an object from a pool.
 *
 * @return
 *      A pointer to the allocated object, or NULL if the pool doesn't have any free objects
 *      to allocate.
 */
//--------------------------------------------------------------------------------------------------
void* le_mem_TryAlloc
(
    le_mem_PoolRef_t    pool    ///< [IN] The pool from which the object is to be allocated.
)
{
    LE_ASSERT(pool != NULL);

    MemBlock_t* blockPtr = NULL;
    void* userPtr = NULL;

    mem_Lock();

#if LE_CONFIG_MEM_POOLS
    // Pop a link off the pool.
    le_sls_Link_t* blockLinkPtr = le_sls_Pop(&(pool->freeList));

    if (blockLinkPtr != NULL)
    {
        // Get the block from the block link.
        blockPtr = CONTAINER_OF(blockLinkPtr, MemBlock_t, data[0].link);
    }
#else
    blockPtr = malloc(pool->blockSize);

    if (blockPtr != NULL)
    {
        InitBlock(pool, blockPtr);
    }
#endif

    if (blockPtr != NULL)
    {
        // Update the pool and the block.
        pool->numBlocksInUse++;
#if LE_CONFIG_MEM_POOL_STATS
    pool->numAllocations++;

    if (pool->numBlocksInUse > pool->maxNumBlocksUsed)
    {
        pool->maxNumBlocksUsed = pool->numBlocksInUse;
    }
#endif

        blockPtr->refCount = 1;

        // Return the user object in the block.
#if LE_CONFIG_USE_GUARD_BAND
        InitGuardBands(blockPtr);
        userPtr = &blockPtr->data[0].item + GUARD_BAND_SIZE;
#else
        userPtr = blockPtr->data;
#endif
    }

    mem_Unlock();

    return userPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Allocates an object from a pool or logs a fatal error and terminates the process if the pool
 * doesn't have any free objects to allocate.
 *
 * @return A pointer to the allocated object.
 *
 * @note    On failure, the process exits, so you don't have to worry about checking the returned
 *          pointer for validity.
 */
//--------------------------------------------------------------------------------------------------
void* le_mem_AssertAlloc
(
    le_mem_PoolRef_t    pool    ///< [IN] The pool from which the object is to be allocated.
)
{
    LE_ASSERT(pool != NULL);

    void* objPtr = le_mem_TryAlloc(pool);

    LE_ASSERT(objPtr);

    return objPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Allocates an object from a pool or logs a warning and expands the pool if the pool
 * doesn't have any free objects to allocate.
 *
 * @return  A pointer to the allocated object.
 *
 * @note    On failure, the process exits, so you don't have to worry about checking the returned
 *          pointer for validity.
 */
//--------------------------------------------------------------------------------------------------
void* le_mem_ForceAlloc
(
    le_mem_PoolRef_t    pool    ///< [IN] The pool from which the object is to be allocated.
)
{
    LE_ASSERT(pool != NULL);

    void* objPtr;

#if LE_CONFIG_MEM_POOLS
    while ((objPtr = le_mem_TryAlloc(pool)) == NULL)
    {
        // Expand the pool.
        le_mem_ExpandPool(pool, pool->numBlocksToForce);

        mem_Lock();
#    if LE_CONFIG_MEM_POOL_STATS
        pool->numOverflows++;
#    endif

            // log a warning.
#   if !LE_CONFIG_LINUX
        LE_WARN("Memory pool '%s' overflowed. Expanded to %" PRIuS " blocks.",
            MEMPOOL_NAME(pool->name), pool->totalBlocks);
#   else
        LE_DEBUG("Memory pool '%s' overflowed. Expanded to %" PRIuS " blocks.",
            MEMPOOL_NAME(pool->name), pool->totalBlocks);
#   endif
            mem_Unlock();

        }
#else /* !LE_CONFIG_MEM_POOLS */
        objPtr = le_mem_AssertAlloc(pool);
#endif /* end !LE_CONFIG_MEM_POOLS */

    return objPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Attempt to get the pool from which a block should be allocated.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t GetPoolForSize
(
    le_mem_PoolRef_t pool,    ///< [IN] Pool (possibly restricted size pool) from which to allocate
    size_t size               ///< [IN] The size of block to allocate
)
{
    while (pool->userDataSize < size)
    {
        if (pool->superPoolPtr && pool->superPoolPtr->userDataSize > pool->userDataSize)
        {
            pool = pool->superPoolPtr;
        }
        else
        {
            LE_FATAL("Attempting to allocate block of size %"PRIuS" from pool with max size %"PRIuS,
                     size, pool->userDataSize);
        }
    }

    return pool;
}


//--------------------------------------------------------------------------------------------------
/**
 * Attempts to allocate an object of a specific size from a pool.
 *
 * @return
 *      A pointer to the allocated object, or NULL if the pool doesn't have any free objects
 *      to allocate.
 */
//--------------------------------------------------------------------------------------------------
void* le_mem_TryVarAlloc
(
    le_mem_PoolRef_t    pool,    ///< [IN] The pool from which the object is to be allocated.
    size_t              size     ///< [IN] The size of block to allocate
)
{
    LE_ASSERT(pool != NULL);

    return le_mem_TryAlloc(GetPoolForSize(pool, size));
}


//--------------------------------------------------------------------------------------------------
/**
 * Allocates an object of a specific size from a pool or logs a fatal error and terminates the
 *  process if the pool doesn't have any free objects to allocate.
 *
 * @return A pointer to the allocated object.
 *
 * @note    On failure, the process exits, so you don't have to worry about checking the returned
 *          pointer for validity.
 */
//--------------------------------------------------------------------------------------------------
void* le_mem_AssertVarAlloc
(
    le_mem_PoolRef_t    pool,    ///< [IN] The pool from which the object is to be allocated.
    size_t              size     ///< [IN] The size of block to allocate
)
{
    LE_ASSERT(pool != NULL);

    return le_mem_AssertAlloc(GetPoolForSize(pool, size));
}


//--------------------------------------------------------------------------------------------------
/**
 * Allocates an object of a specific size from a pool or logs a warning and expands the pool if the
 * pool doesn't have any free objects to allocate.
 *
 * @return  A pointer to the allocated object.
 *
 * @note    On failure, the process exits, so you don't have to worry about checking the returned
 *          pointer for validity.
 */
//--------------------------------------------------------------------------------------------------
void* le_mem_ForceVarAlloc
(
    le_mem_PoolRef_t    pool,    ///< [IN] The pool from which the object is to be allocated.
    size_t              size     ///< [IN] The size of block to allocate
)
{
    LE_ASSERT(pool != NULL);

    return le_mem_ForceAlloc(GetPoolForSize(pool, size));
}

//--------------------------------------------------------------------------------------------------
/**
 * Fetches the size of a block (in bytes).
 *
 * @return
 *      Object size, in bytes.
 */
//--------------------------------------------------------------------------------------------------
size_t le_mem_GetBlockSize
(
    void* objPtr                 ///< [IN] Pointer to the object to get size of.
)
{
    LE_ASSERT(objPtr);

    return _le_mem_GetBlockPool(objPtr)->userDataSize;
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the number of objects that is added when le_mem_ForceAlloc expands the pool.
 *
 * @return
 *      Nothing.
 *
 * @note
 *      The default value is one.
 */
//--------------------------------------------------------------------------------------------------
void le_mem_SetNumObjsToForce
(
    le_mem_PoolRef_t    pool,       ///< [IN] The pool to set the number of objects for.
    size_t              numObjects  ///< [IN] The number of objects that is added when
                                    ///       le_mem_ForceAlloc expands the pool.
)
{
    LE_ASSERT(pool != NULL);

    // Do not allow forcing less than 1 object
    if (numObjects <= 0)
    {
        numObjects = 1;
    }

    mem_Lock();
    pool->numBlocksToForce = numObjects;
    mem_Unlock();
}


//--------------------------------------------------------------------------------------------------
/**
 * Releases an object.  If the object's reference count has reached zero, it will be destructed
 * and its memory will be put back into the pool for later reuse.
 *
 * @return
 *      Nothing.
 *
 * @warning
 *      - <b>Do not EVER access an object after releasing it.</b>  It might not exist anymore.
 *      - If the object has a destructor that accesses a data structure that is shared by multiple
 *        threads, make sure you hold the mutex (or take other measures to prevent races) before
 *        releasing the object.
 */
//--------------------------------------------------------------------------------------------------
void le_mem_Release
(
    void*   objPtr  ///< [IN] Pointer to the object to be released.
)
{
    MemBlock_t* blockPtr;

    // Get the block from the object pointer.
#if LE_CONFIG_USE_GUARD_BAND
    uint8_t* dataPtr = objPtr;
    dataPtr -= GUARD_BAND_SIZE;
    blockPtr = CONTAINER_OF(dataPtr, MemBlock_t, data);
#else
    blockPtr = CONTAINER_OF(objPtr, MemBlock_t, data);
#endif

#if LE_CONFIG_USE_GUARD_BAND
    CheckGuardBands(blockPtr);
#endif

    mem_Lock();

    switch (blockPtr->refCount)
    {
        case 1:
        {
            le_mem_Pool_t* poolPtr = blockPtr->poolPtr;

            // The reference count has reached zero.
            blockPtr->refCount = 0;

            // Call the destructor, if there is one.
            if (poolPtr->destructor)
            {
                // Make sure that the destructor is not called with the mutex locked, because
                // it is not a recursive mutex and therefore will deadlock if locked again by
                // the same thread.  Also, fetch the destructor function address before unlocking
                // the mutex so that we don't touch the pool object while the mutex is unlocked.
                le_mem_Destructor_t destructor = poolPtr->destructor;
                mem_Unlock();
                destructor(objPtr);

                // Re-lock the mutex now so that it is safe to access the pool object again.
                mem_Lock();
            }

#if LE_CONFIG_MEM_POOLS
            // Release the memory back into the pool.
            // Note that we don't do this before calling the destructor because the destructor
            // still needs to access it, but after it goes back on the free list, it could get
            // reallocated by another thread (or even the destructor itself) and have its
            // contents clobbered.
            blockPtr->data[0].link = LE_SLS_LINK_INIT;
            le_sls_Stack(&(poolPtr->freeList), &(blockPtr->data[0].link));
#else
            free(blockPtr);
#endif

            poolPtr->numBlocksInUse--;

            break;
        }

        case 0:
            LE_EMERG("Releasing free block.");
            LE_FATAL("Free block released from pool %p (%s).",
                     blockPtr->poolPtr,
                     MEMPOOL_NAME(blockPtr->poolPtr->name));
            break;

        default:
            blockPtr->refCount--;
    }

    mem_Unlock();
}


//--------------------------------------------------------------------------------------------------
/**
 * Increments the reference count on an object by 1.
 *
 * See @ref ref_counting for more information.
 *
 * @return
 *      Nothing.
 */
//--------------------------------------------------------------------------------------------------
void le_mem_AddRef
(
    void*   objPtr  ///< [IN] Pointer to the object.
)
{
#if LE_CONFIG_USE_GUARD_BAND
    objPtr = (((uint8_t*)objPtr) - GUARD_BAND_SIZE);
#endif
    MemBlock_t* memBlockPtr = CONTAINER_OF(objPtr, MemBlock_t, data);

#if LE_CONFIG_USE_GUARD_BAND
    CheckGuardBands(memBlockPtr);
#endif

    mem_Lock();

    LE_ASSERT(memBlockPtr->refCount != 0);

    memBlockPtr->refCount++;

    mem_Unlock();
}


//----------------------------------------------------------------------------------------------
/**
 * Fetches the reference count on an object.
 *
 * See @ref mem_ref_counting for more information.
 *
 * @return
 *      The reference count on the object.
 */
//----------------------------------------------------------------------------------------------
size_t le_mem_GetRefCount
(
    void*   objPtr  ///< [IN] Pointer to the object.
)
{
#if LE_CONFIG_USE_GUARD_BAND
    objPtr = (((uint8_t*)objPtr) - GUARD_BAND_SIZE);
#endif
    MemBlock_t* memBlockPtr = CONTAINER_OF(objPtr, MemBlock_t, data);

    return memBlockPtr->refCount;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the destructor function for a given pool.
 *
 * See @ref destructors for more information.
 *
 * @return
 *      Nothing.
 */
//--------------------------------------------------------------------------------------------------
void le_mem_SetDestructor
(
    le_mem_PoolRef_t    pool,       ///< [IN] The pool.
    le_mem_Destructor_t destructor  ///< [IN] The destructor function.
)
{
    LE_ASSERT(pool != NULL);

    mem_Lock();
    pool->destructor = destructor;
    mem_Unlock();
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the statistics for a given pool.
 *
 * @return
 *      Nothing.  Uses output parameter instead.
 */
//--------------------------------------------------------------------------------------------------
void le_mem_GetStats
(
    le_mem_PoolRef_t    pool,       ///< [IN] The pool whose stats are to be fetched.
    le_mem_PoolStats_t* statsPtr    ///< [OUT] Pointer to where the stats will be stored.
)
{
    LE_ASSERT( (pool != NULL) && (statsPtr != NULL) );

    mem_Lock();

#if LE_CONFIG_MEM_POOL_STATS
    statsPtr->numAllocs = pool->numAllocations;
    statsPtr->numOverflows = pool->numOverflows;
    statsPtr->maxNumBlocksUsed = pool->maxNumBlocksUsed;
#else
    statsPtr->numAllocs = 0;
    statsPtr->numOverflows = 0;
    statsPtr->maxNumBlocksUsed = 0;
#endif
    statsPtr->numFree = pool->totalBlocks - pool->numBlocksInUse;
    statsPtr->numBlocksInUse = pool->numBlocksInUse;

    mem_Unlock();
}


//--------------------------------------------------------------------------------------------------
/**
 * Resets the statistics for a given pool.
 *
 * @return
 *      Nothing.
 */
//--------------------------------------------------------------------------------------------------
void le_mem_ResetStats
(
    le_mem_PoolRef_t    pool        ///< [IN] The pool whose stats are to be reset.
)
{
    LE_ASSERT(pool != NULL);

#if LE_CONFIG_MEM_POOL_STATS
    mem_Lock();
    pool->numAllocations = 0;
    pool->numOverflows = 0;
    mem_Unlock();
#endif
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the memory pool's name, including the component name prefix.
 *
 * If the pool were given the name "myPool" and the component that it belongs to is called
 * "myComponent", then the full pool name returned by this function would be "myComponent.myPool".
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the name was truncated to fit in the provided buffer.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mem_GetName
(
    le_mem_PoolRef_t    pool,       ///< [IN] The memory pool.
    char*               namePtr,    ///< [OUT] Buffer to store the name of memory pool.
    size_t              bufSize     ///< [IN] Size of the buffer namePtr points to.
)
{
    LE_ASSERT(pool != NULL);

#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
    mem_Lock();
    le_result_t result = le_utf8_Copy(namePtr, pool->name, bufSize, NULL);
    mem_Unlock();
#else /* if not LE_CONFIG_MEM_POOL_NAMES_ENABLED */
    le_result_t result = le_utf8_Copy(namePtr, "<omitted>", bufSize, NULL);
#endif /* end LE_CONFIG_MEM_POOL_NAMES_ENABLED */

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the specified pool is a sub-pool.
 *
 * @return
 *      true if it is a sub-pool.
 *      false if it is not a sub-pool.
 */
//--------------------------------------------------------------------------------------------------
bool le_mem_IsSubPool
(
    le_mem_PoolRef_t    pool        ///< [IN] The memory pool.
)
{
    LE_ASSERT(pool != NULL);

    bool isSubPool;

    mem_Lock();
    isSubPool = (pool->superPoolPtr != NULL);
    mem_Unlock();

    return isSubPool;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the number of objects that a given pool can hold (this includes both the number of
 * free and in-use objects).
 *
 * @return
 *      Total number of objects.
 */
//--------------------------------------------------------------------------------------------------
size_t le_mem_GetObjectCount
(
    le_mem_PoolRef_t    pool        ///< [IN] The pool whose number of objects is to be fetched.
)
{
    LE_ASSERT(pool != NULL);

    size_t numBlocks;

    mem_Lock();
    numBlocks = pool->totalBlocks;
    mem_Unlock();

    return numBlocks;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the size of the objects in a given pool (in bytes).
 *
 * @return
 *      Object size, in bytes.
 */
//--------------------------------------------------------------------------------------------------
size_t le_mem_GetObjectSize
(
    le_mem_PoolRef_t    pool        ///< [IN] The pool whose object size is to be fetched.
)
{
    LE_ASSERT(pool != NULL);

    size_t objSize;

    mem_Lock();
    objSize = pool->userDataSize;
    mem_Unlock();

    return objSize;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the total size of the object including all the memory overhead in a given pool (in bytes).
 *
 * @return
 *      Total object memory size, in bytes.
 */
//--------------------------------------------------------------------------------------------------
size_t le_mem_GetObjectFullSize
(
    le_mem_PoolRef_t    pool        ///< [IN] The pool whose object memory size is to be fetched.
)
{
    LE_ASSERT(pool != NULL);

    size_t objSize;

    mem_Lock();
    objSize = pool->blockSize;
    mem_Unlock();

    return objSize;
}


#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/**
 * Finds a pool given the pool's name.
 *
 * @return
 *      Reference to the pool, or NULL if the pool doesn't exist.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t _le_mem_FindPool
(
    const char*     componentName,  ///< [IN] Name of the component.
    const char*     name            ///< [IN] Name of the pool inside the component.
)
{
    le_mem_PoolRef_t result = NULL;

    // Construct the component-scoped pool name.
    // Note: Don't check for truncation because if it is truncated, it will be consistent with
    //       the truncation that would have occurred in InitPool().
    char fullName[MAX_POOL_NAME_BYTES];
    (void)snprintf(fullName, sizeof(fullName), "%s.%s", componentName, name);

    mem_Lock();

    // Search all pools except for the first one because the first pool is always the sub-pools pool.
    le_dls_Link_t* poolLinkPtr = le_dls_Peek(&PoolList);
    poolLinkPtr = le_dls_PeekNext(&PoolList, poolLinkPtr);

    while (poolLinkPtr)
    {
        le_mem_Pool_t* memPoolPtr = CONTAINER_OF(poolLinkPtr, le_mem_Pool_t, poolLink);

        if (strcmp(fullName, memPoolPtr->name) == 0)
        {
            result = memPoolPtr;
            break;
        }

        poolLinkPtr = le_dls_PeekNext(&PoolList, poolLinkPtr);
    }

    mem_Unlock();

    return result;
}
#endif /* end LE_CONFIG_MEM_POOL_NAMES_ENABLED */



//--------------------------------------------------------------------------------------------------
/**
 * Creates a sub-pool.  You cannot create sub-pools of sub-pools so do not attempt to pass a
 * sub-pool in the superPool parameter.
 *
 * See @ref sub_pools for more information.
 *
 * @return
 *      A reference to the sub-pool.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
le_mem_PoolRef_t _le_mem_CreateSubPool
(
    le_mem_PoolRef_t    superPool,      ///< [IN] The super-pool.
    const char*         componentName,  ///< [IN] Name of the component.
    const char*         name,           ///< [IN] Name of the sub-pool (will be copied into the
                                        ///   sub-pool).
    size_t              numObjects      ///< [IN] The number of objects to take from the super-pool.
)
#else
le_mem_PoolRef_t _le_mem_CreateSubPool
(
    le_mem_PoolRef_t    superPool,      ///< [IN] Super-pool.
    size_t              numObjects      ///< [IN] Number of objects to take from the super-pool.
)
#endif
{
    LE_ASSERT(superPool != NULL);

    // Get a sub-pool from the pool of sub-pools.
    le_mem_PoolRef_t subPool = le_mem_ForceAlloc(SubPoolsPool);
    memset(subPool, 0, sizeof(le_mem_Pool_t));

    // Initialize the pool.
#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
    InitPool(subPool, componentName, name, superPool->userDataSize);
#else
    InitPool(subPool, superPool->userDataSize);
#endif
    subPool->superPoolPtr = superPool;

    mem_Lock();

    // Log an error if the pool name is not unique.
    VerifyUniquenessOfName(subPool);

    // Add the sub-pool to the list of pools.
    PoolListChangeCount++;
    le_dls_Queue(&PoolList, &(subPool->poolLink));

    mem_Unlock();

    // Expand the pool to its initial size.
    // Note:    This moves blocks from the parent pool to the sub pool, expanding the parent pool,
    //          if necessary.
    le_mem_ExpandPool(subPool, numObjects);

    // Inherit the parent pool's destructor.
    subPool->destructor = superPool->destructor;

    return subPool;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a sub-pool of smaller objects.
 *
 * See @ref mem_reduced_pools for more information.
 *
 * @return
 *      Reference to the sub-pool.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
le_mem_PoolRef_t _le_mem_CreateReducedPool
(
    le_mem_PoolRef_t    superPool,      ///< [IN] Super-pool.
    const char*         componentName,  ///< [IN] Name of the component.
    const char*         name,           ///< [IN] Name of the sub-pool (will be copied into the
                                        ///   sub-pool).
    size_t              numObjects,     ///< [IN] Minimum number of objects in the subpool
                                        ///< by default.
    size_t              objSize         ///< [IN] Minimum size of objects in the subpool.
)
#else
le_mem_PoolRef_t _le_mem_CreateReducedPool
(
    le_mem_PoolRef_t    superPool,      ///< [IN] Super-pool.
    size_t              numObjects,     ///< [IN] Minimum number of objects in the subpool
                                        ///< by default.
    size_t              objSize         ///< [IN] Minimum size of objects in the subpool.
)
#endif
{
    LE_ASSERT(superPool != NULL);

    LE_FATAL_IF(objSize > superPool->userDataSize,
                "Subpool object size must be smaller than parent object size");

    // Calculate object size -- first find how many of the objects can fit in each block of the
    // parent pool
    size_t subpoolBlockSize = CalcBlockSize(objSize);
    size_t divisor = superPool->blockSize/subpoolBlockSize;

    if (divisor == 1 && superPool->superPoolPtr)
    {
        // If this is already a subpool and not being further subdivided, just increase
        // the reference count and return a pointer to the existing pool.
        le_mem_AddRef(superPool);

        mem_Lock();

        if (numObjects > superPool->totalBlocks)
        {
            ExpandPool_NoLock(superPool, numObjects - superPool->totalBlocks);
        }

        mem_Unlock();

        return superPool;
    }

    // Get a sub-pool from the pool of sub-pools.
    le_mem_PoolRef_t subPool = le_mem_ForceAlloc(SubPoolsPool);
    memset(subPool, 0, sizeof(le_mem_Pool_t));

    // Then use the maximum object size for such a block as the object size for the sub-pool.
    size_t realObjSize = superPool->blockSize/divisor - sizeof(MemBlock_t) - 2*GUARD_BAND_SIZE;
    realObjSize -= (realObjSize % sizeof(size_t));

    // Initialize the pool.
#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
    InitPool(subPool, componentName, name, realObjSize);
#else
    InitPool(subPool, realObjSize);
#endif
    subPool->superPoolPtr = superPool;

    // Verify sub-pool block size is correct compared with parent pool.
    LE_FATAL_IF((superPool->blockSize / subPool->blockSize) != divisor,
                "superPool->blockSize (%"PRIuS") / subPool->blockSize (%"PRIuS") != "
                "divisor (%"PRIuS")",
                superPool->blockSize, subPool->blockSize, divisor);

    mem_Lock();

    // Log an error if the pool name is not unique.
    VerifyUniquenessOfName(subPool);

    // Add the sub-pool to the list of pools.
    PoolListChangeCount++;
    le_dls_Queue(&PoolList, &(subPool->poolLink));

    mem_Unlock();

    // Expand the pool to its initial size.
    // Note:    This moves blocks from the parent pool to the sub pool, expanding the parent pool,
    //          if necessary.
    le_mem_ExpandPool(subPool, numObjects);

    // Inherit the parent pool's destructor.
    subPool->destructor = superPool->destructor;

    return subPool;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a sub-pool.
 *
 * See @ref sub_pools for more information.
 *
 * @return
 *      Nothing.
 */
//--------------------------------------------------------------------------------------------------
void le_mem_DeleteSubPool
(
    le_mem_PoolRef_t    subPool     ///< [IN] The sub-pool to be deleted.
)
{
    LE_ASSERT(subPool != NULL);

    // Release the sub-pool.
    le_mem_Release(subPool);
}

#if LE_CONFIG_RTOS

//--------------------------------------------------------------------------------------------------
/**
 * Size of a compact free block
 *
 * A compact free block consists only of the header and a link to the next compact free block
 */
//--------------------------------------------------------------------------------------------------
#define COMPACT_FREE_BLOCK_SIZE   (sizeof(MemBlock_t) + sizeof(le_sls_Link_t))

//--------------------------------------------------------------------------------------------------
/**
 * Start of pool memory
 *
 * @note The extern is required to access symbols defined only in the linker script.
 */
//--------------------------------------------------------------------------------------------------
extern uint8_t le_mem_StartPools[];


//--------------------------------------------------------------------------------------------------
/**
 * End of pool memory
 *
 * @note The extern is required to access symbols defined only in the linker script.
 */
//--------------------------------------------------------------------------------------------------
extern uint8_t le_mem_EndPools[];


//--------------------------------------------------------------------------------------------------
/**
 * End of compacted memory pools.
 *
 * This is valid only after calling le_mem_Hibernate() but before calling le_mem_Resume()
 */
//--------------------------------------------------------------------------------------------------
static uint8_t *EndOfHibernationPtr;

//--------------------------------------------------------------------------------------------------
/**
 * List of compact free memory areas.
 *
 * Non-empty only after calling le_mem_Hibernate() but before calling le_mem_Resume()
 */
//--------------------------------------------------------------------------------------------------
static le_sls_List_t CompactBlockList = LE_SLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Compare two linked list elements by link address.
 *
 * Used for sorting free lists by address in preparation for compressing the free space
 */
//--------------------------------------------------------------------------------------------------
static bool CompareLinkAddr(
    le_sls_Link_t* a,
    le_sls_Link_t* b
)
{
    return a < b;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a free block record in the compacted memory pool area
 */
//--------------------------------------------------------------------------------------------------
void SpillFreeBlocks
(
    MemBlock_t *freeBlockPtr
)
{
    MemBlock_t *compactedFreeBlockPtr = (MemBlock_t*)EndOfHibernationPtr;

    // Create a new compact free block record
    memcpy(compactedFreeBlockPtr, freeBlockPtr, sizeof(MemBlock_t));
    compactedFreeBlockPtr->data[0].link = LE_SLS_LINK_INIT;

    // Add it to the list of compact free blocks
    le_sls_Stack(&CompactBlockList, &compactedFreeBlockPtr->data[0].link);

    // Move compact free memory forward
    EndOfHibernationPtr += COMPACT_FREE_BLOCK_SIZE;

    // And reset the free block
    freeBlockPtr->poolPtr = NULL;
    freeBlockPtr->refCount = 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Compress memory pools ready for hibernate-to-RAM
 *
 * This compresses the memory pools ready for hibernation.  All Legato tasks must remain
 * suspended until after le_mem_Resume() is called.
 *
 * @return Nothing
 */
//--------------------------------------------------------------------------------------------------
void le_mem_Hibernate
(
    void **freeStartPtr,     ///< [OUT] Beginning of unused memory which does not need to be
                             ///<       preserved in hibernation
    void **freeEndPtr        ///< [OUT] End of unused memory
)
{
    // No need to lock -- other Legato threads must not be running now anyway.
    le_mem_Pool_t* currentPoolPtr;
    le_sls_List_t allFreeList = LE_SLS_LIST_INIT;

    // Collect all the free items from the pools
    LE_DLS_FOREACH(&PoolList, currentPoolPtr, le_mem_Pool_t, poolLink)
    {
        le_sls_Link_t *curLink = le_sls_Peek(&currentPoolPtr->freeList);
        le_sls_Link_t *prevLink = NULL;
        while (curLink)
        {
            // Save next node as we might be removing the current item.
            le_sls_Link_t* nextLink = le_sls_PeekNext(&currentPoolPtr->freeList,
                                                      curLink);
            MemBlock_t* currentFreeBlockPtr =
                CONTAINER_OF(curLink, MemBlock_t, data[0].link);

            // Is this in a static pool?
            if (((uint8_t*)currentFreeBlockPtr >= le_mem_StartPools) &&
                ((uint8_t*)currentFreeBlockPtr < le_mem_EndPools))
            {
                le_sls_RemoveAfter(&currentPoolPtr->freeList, prevLink);
                le_sls_Stack(&allFreeList, curLink);

                // Current node is removed from list, so previous node doesn't change.
            }
            else
            {
                LE_DEBUG("Free %"PRIuS"B heap block at %p when hibernating",
                         currentPoolPtr->blockSize, curLink);
                prevLink = curLink;
            }

            curLink = nextLink;
        }
    }

    // Sort by address (smallest to largest)
    le_sls_Sort(&allFreeList, CompareLinkAddr);

    // Combine adjacent free blocks which are part of the same pool, keeping just the headers,
    // and compact used memory.
    MemBlock_t* currentBlockPtr;
    MemBlock_t freeBlock = { .poolPtr = NULL, .refCount = 0 };
    uint8_t* nextDecompactedMemPtr = le_mem_StartPools;

    // Reset end of memory to be preserved in hibernation
    EndOfHibernationPtr = le_mem_StartPools;

    LE_SLS_FOREACH(&allFreeList, currentBlockPtr, MemBlock_t, data[0].link)
    {
        if ((void*)currentBlockPtr != (void*)nextDecompactedMemPtr)
        {
            // There's used memory since the last free block.  Spill any free blocks,
            // then copy in used memory to preserve
            if (freeBlock.poolPtr)
            {
                SpillFreeBlocks(&freeBlock);

                // Ensure there's no overflow into not-yet compacted memory after spilling
                LE_ASSERT(EndOfHibernationPtr <= nextDecompactedMemPtr);
            }

            size_t usedBlockSize = (uint8_t*)currentBlockPtr - nextDecompactedMemPtr;
            memmove(EndOfHibernationPtr, nextDecompactedMemPtr,
                    usedBlockSize);
            EndOfHibernationPtr += usedBlockSize;
            nextDecompactedMemPtr += usedBlockSize;
        }

        if (freeBlock.poolPtr && (freeBlock.poolPtr != currentBlockPtr->poolPtr))
        {
            // Moving to a new pool; spill free blocks
            SpillFreeBlocks(&freeBlock);

            // Ensure there's no overflow into not-yet compacted memory after spilling
            LE_ASSERT(EndOfHibernationPtr <= nextDecompactedMemPtr);
        }

        // Now either there is no current pool, or it's the same as the existing pool.
        if (!freeBlock.poolPtr)
        {
            freeBlock.poolPtr = currentBlockPtr->poolPtr;
            // In a compacted pool area, ref count is the number of free blocks in a row.
            freeBlock.refCount = 1;
        }
        else
        {
            freeBlock.refCount++;
        }

        // Move nextDecompactedMemPtr past this block
        nextDecompactedMemPtr += currentBlockPtr->poolPtr->blockSize;
    }

    // Finally spill the last free block (if one is in progress)
    if (freeBlock.poolPtr)
    {
        SpillFreeBlocks(&freeBlock);

        // Ensure there's no overflow into not-yet compacted memory after spilling
        LE_ASSERT(EndOfHibernationPtr <= nextDecompactedMemPtr);
    }

    // and compact any final used memory
    size_t usedBlockSize = (uint8_t*)le_mem_EndPools - nextDecompactedMemPtr;
    memmove(EndOfHibernationPtr, nextDecompactedMemPtr,
            usedBlockSize);
    EndOfHibernationPtr += usedBlockSize;
    nextDecompactedMemPtr += usedBlockSize;

    // Ensure all pool memory is compacted
    LE_ASSERT(nextDecompactedMemPtr == le_mem_EndPools);

    if (freeStartPtr)
    {
        *freeStartPtr = EndOfHibernationPtr;
    }

    if (freeEndPtr)
    {
        *freeEndPtr = le_mem_EndPools;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Decompress memory pools after waking from hibernate-to-RAM
 *
 * This decompresses memory pools after hibernation.  After this function returns, Legato tasks
 * may be resumed.
 *
 * @return Nothing
 */
//--------------------------------------------------------------------------------------------------
void le_mem_Resume
(
    void
)
{
    le_sls_Link_t* compactBlockLinkPtr;
    uint8_t *startOfDecompactedMem = le_mem_EndPools;

    while ((compactBlockLinkPtr = le_sls_Pop(&CompactBlockList)) != NULL)
    {
        MemBlock_t* compactBlockPtr =
            CONTAINER_OF(compactBlockLinkPtr, MemBlock_t, data[0].link);
        uint8_t* endOfBlock = (uint8_t*)compactBlockPtr + COMPACT_FREE_BLOCK_SIZE;

        if (endOfBlock != EndOfHibernationPtr)
        {
            // There is some used memory after this free block, move it into place.
            size_t usedMemSize = EndOfHibernationPtr - endOfBlock;
            startOfDecompactedMem -= usedMemSize;
            EndOfHibernationPtr -= usedMemSize;
            memmove(startOfDecompactedMem, EndOfHibernationPtr, usedMemSize);
        }

        le_mem_Pool_t* currentPoolPtr = compactBlockPtr->poolPtr;
        size_t blockSize = currentPoolPtr->blockSize;
        size_t i;

        // Decompact all free blocks.
        for (i = 0; i < compactBlockPtr->refCount; ++i)
        {
            startOfDecompactedMem -= blockSize;
            MemBlock_t* currentBlock = (MemBlock_t*)startOfDecompactedMem;
            currentBlock->poolPtr = currentPoolPtr;
            currentBlock->refCount = 0;
            currentBlock->data[0].link = LE_SLS_LINK_INIT;
            le_sls_Stack(&currentPoolPtr->freeList, &currentBlock->data[0].link);
        }

        // Update end of hibernation memory
        EndOfHibernationPtr = (uint8_t*)compactBlockPtr;
    }
}

#endif /* end LE_CONFIG_RTOS */
