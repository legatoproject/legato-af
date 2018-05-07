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
 * A debugging feature can be enabled at compile-time by defining the macro "USE_GUARD_BAND".
 * This inserts chunks of memory into each memory block both before and after the user object
 * part.  These chunks of memory, called "guard bands", are filled with a special pattern that
 * is unlikely to occur in normal data.  Whenever a block is allocated or released, the
 * guard bands are checked for corruption and any corruption is reported.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#include "legato.h"
#include "mem.h"
#include "limit.h"

#define USE_GUARD_BAND
#define FILL_DELETED_AND_CHECK_ALLOCATED

#define NUM_GUARD_BAND_WORDS 8
#define GUARD_WORD ((uint32_t)0xDEADBEEF)
#define GUARD_BAND_SIZE (sizeof(GUARD_WORD) * NUM_GUARD_BAND_WORDS)


/// The maximum total pool name size, including the component prefix, which is a component
/// name plus a '.' separator ("myComp.myPool") and the null terminator.
#define MAX_POOL_NAME_BYTES (LIMIT_MAX_COMPONENT_NAME_LEN + 1 + LIMIT_MAX_MEM_POOL_NAME_BYTES)

/// The default number of Sub Pool objects in the Sub Pools Pool.
/// @todo Make this configurable.
#define DEFAULT_SUB_POOLS_POOL_SIZE     8


//--------------------------------------------------------------------------------------------------
/**
 * The default number of blocks to expand by when the le_mem_ForceAlloc expands the memory pool.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_NUM_BLOCKS_TO_FORCE     1


#ifdef LE_MEM_TRACE
    #undef le_mem_TryAlloc
    #undef le_mem_AssertAlloc
    #undef le_mem_ForceAlloc
    #undef le_mem_Release
    #undef le_mem_AddRef

    #define le_mem_TryAlloc _le_mem_TryAlloc
    #define le_mem_AssertAlloc _le_mem_AssertAlloc
    #define le_mem_ForceAlloc _le_mem_ForceAlloc
    #define le_mem_Release _le_mem_Release
    #define le_mem_AddRef _le_mem_AddRef
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Definition of a memory block.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    #ifndef LE_MEM_VALGRIND
        le_sls_Link_t link;         ///< This block's link in the memory pool.
                                    ///  NOTE: Only used while free.
    #endif

    MemPool_t* poolPtr;         ///< A pointer to the pool (or sub-pool) that this block belongs to.

    size_t refCount;            ///< The number of external references to this memory block's
                                ///     user object. (0 = free)

    uint8_t  data[];            ///< This block's data content (Has a guard band at the
                                ///     start and end if USE_GUARD_BAND is defined).
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
static inline void Lock
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
static inline void Unlock
(
    void
)
{
    LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0);
}


#ifdef USE_GUARD_BAND

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
        for (i = 0; i < NUM_GUARD_BAND_WORDS; i++, guardBandWordPtr++)
        {
            *guardBandWordPtr = GUARD_WORD;
        }

        // There's another guard band at the end of the data section.
        guardBandWordPtr = (uint32_t*)(   ((uint8_t*)blockHeaderPtr)
                                        + blockHeaderPtr->poolPtr->blockSize
                                        - GUARD_BAND_SIZE );
        for (i = 0; i < NUM_GUARD_BAND_WORDS; i++, guardBandWordPtr++)
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
        for (i = 0; i < NUM_GUARD_BAND_WORDS; i++, guardBandWordPtr++)
        {
            if (*guardBandWordPtr != GUARD_WORD)
            {
                LE_EMERG("Memory corruption detected at address %p before object allocated"
                                                                                " from pool '%s'.",
                         guardBandWordPtr,
                         blockHeaderPtr->poolPtr->name);
                LE_FATAL("Guard band value should have been %d, but was found to be %d.",
                         GUARD_WORD,
                         *guardBandWordPtr);
            }
        }

        // There's another guard band at the end of the data section.
        guardBandWordPtr = (uint32_t*)(   ((uint8_t*)blockHeaderPtr)
                                        + blockHeaderPtr->poolPtr->blockSize
                                        - GUARD_BAND_SIZE);
        for (i = 0; i < NUM_GUARD_BAND_WORDS; i++, guardBandWordPtr++)
        {
            if (*guardBandWordPtr != GUARD_WORD)
            {
                LE_EMERG("Memory corruption detected at address %p at end of object allocated"
                                                                                " from pool '%s'.",
                         guardBandWordPtr,
                         blockHeaderPtr->poolPtr->name);
                LE_FATAL("Guard band value should have been %d, but was found to be %d.",
                         GUARD_WORD,
                         *guardBandWordPtr);
            }
        }
    }

#endif


//--------------------------------------------------------------------------------------------------
/**
 * Initializes a memory pool.
 *
 * @warning Called without the mutex locked.
 */
//--------------------------------------------------------------------------------------------------
static void InitPool
(
    le_mem_PoolRef_t    pool,       ///< [IN] The pool to initialize.
    const char*      componentName, ///< [IN] Name of the component.
    const char*         name,       ///< [IN] Name of the pool inside the component.
    size_t              objSize     ///< [IN] The size of the individual objects to be allocated
                                    ///       from this pool in bytes.
)
{
    // Construct the component-scoped pool name.
    size_t nameSize = snprintf(pool->name, sizeof(pool->name), "%s.%s", componentName, name);
    if (nameSize >= sizeof(pool->name))
    {
        LE_DEBUG("Memory pool name '%s.%s' is truncated to '%s'", componentName, name, pool->name);
    }

    // Compute the total block size.
    size_t blockSize = sizeof(MemBlock_t) + objSize;

    #ifdef USE_GUARD_BAND
    {
        // Add guard bands around the user data in every block.
        blockSize += (GUARD_BAND_SIZE * 2);
    }
    #endif

    // Round up the block size to the nearest multiple of the processor word size.
    size_t remainder = blockSize % sizeof(void*);
    if (remainder != 0)
    {
        blockSize += (sizeof(void*) - remainder);
    }

    pool->poolLink = LE_DLS_LINK_INIT;

    #ifndef LE_MEM_VALGRIND
        pool->freeList = LE_SLS_LIST_INIT;
    #endif

    pool->userDataSize = objSize;
    pool->blockSize = blockSize;
    pool->destructor = NULL;
    pool->superPoolPtr = NULL;
    pool->numAllocations = 0;
    pool->numOverflows = 0;
    pool->totalBlocks = 0;
    pool->numBlocksInUse = 0;
    pool->maxNumBlocksUsed = 0;
    pool->numBlocksToForce = DEFAULT_NUM_BLOCKS_TO_FORCE;

    #ifdef LE_MEM_TRACE
        pool->memTrace = NULL;

        if (LE_LOG_SESSION != NULL)
        {
            pool->memTrace = le_log_GetTraceRef(pool->name);
            LE_DEBUG("Tracing enabled for pool '%s'.", pool->name);
        }
    #endif
}


//--------------------------------------------------------------------------------------------------
/**
 * Moves the specified number of blocks from the source pool to the destination pool.
 *
 * @note
 *      Does not update the total number of blocks for either pool.
 *
 * @note
 *      Assumes that the mutex is locked.
 */
//--------------------------------------------------------------------------------------------------
static void MoveBlocks
(
    le_mem_PoolRef_t    destPool,   ///< [IN] The pool to move the blocks to.
    le_mem_PoolRef_t    srcPool,    ///< [IN] The pool to get the blocks from.
    size_t              numBlocks   ///< [IN] The maximum number of blocks to move.
)
{
    #ifndef LE_MEM_VALGRIND
        // Get the first block to move.
        le_sls_Link_t* blockLinkPtr = le_sls_Pop(&(srcPool->freeList));

        size_t i = 0;
        while (i < numBlocks)
        {
            if (blockLinkPtr == NULL)
            {
                LE_FATAL("Asked to move %zu blocks from pool '%s' to pool '%s', "
                         "but only %zu were available.",
                         numBlocks,
                         srcPool->name,
                         destPool->name,
                         i);
            }

            // Add the block to the destination pool.
            le_sls_Stack(&(destPool->freeList), blockLinkPtr);

            // Update the blocks parent pool.
            MemBlock_t* blockPtr = CONTAINER_OF(blockLinkPtr, MemBlock_t, link);
            blockPtr->poolPtr = destPool;

            // Get the next block.
            blockLinkPtr = le_sls_Pop(&(srcPool->freeList));

            i++;
        }
    #endif
}


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
    #ifndef LE_MEM_VALGRIND
        // Add the block to the pool's free list.
        newBlockPtr->link = LE_SLS_LINK_INIT;
        le_sls_Stack(&(pool->freeList), &(newBlockPtr->link));
    #endif

    newBlockPtr->refCount = 0;
    newBlockPtr->poolPtr = pool;

    #ifdef USE_GUARD_BAND
        InitGuardBands(newBlockPtr);
    #endif
}


#ifndef LE_MEM_VALGRIND
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
    le_dls_Link_t* poolLinkPtr = le_dls_Peek(&PoolList);

    while (poolLinkPtr)
    {
        MemPool_t* memPoolPtr = CONTAINER_OF(poolLinkPtr, MemPool_t, poolLink);

        if ((strcmp(newPool->name, memPoolPtr->name) == 0) && (newPool != memPoolPtr))
        {
            LE_WARN("Multiple memory pools share the same name '%s'."
                    " This will become illegal in future releases.\n", memPoolPtr->name);
            break;
        }

        poolLinkPtr = le_dls_PeekNext(&PoolList, poolLinkPtr);
    }
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
    SubPoolsPool = le_mem_CreatePool("SubPools", sizeof(MemPool_t));
    le_mem_ExpandPool(SubPoolsPool, DEFAULT_SUB_POOLS_POOL_SIZE);
}


#ifdef LE_MEM_TRACE
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
        #ifdef USE_GUARD_BAND
            uint8_t* dataPtr = objPtr;
            dataPtr -= GUARD_BAND_SIZE;
            blockPtr = CONTAINER_OF(dataPtr, MemBlock_t, data);
        #else
            blockPtr = CONTAINER_OF(objPtr, MemBlock_t, data);
        #endif

        #ifdef USE_GUARD_BAND
            CheckGuardBands(blockPtr);
        #endif

        return blockPtr->poolPtr;
    }


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
            char poolName[LIMIT_MAX_MEM_POOL_NAME_BYTES] = "";
            LE_ASSERT(le_mem_GetName(pool, poolName, sizeof(poolName)) == LE_OK);

            _le_log_Send((le_log_Level_t)-1,
                         trace,
                         LE_LOG_SESSION,
                         le_path_GetBasenamePtr(file, "/"),
                         callingfunction,
                         line,
                         "%s: %s, %p",
                         poolName,
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
le_mem_PoolRef_t _le_mem_CreatePool
(
    const char*     componentName,  ///< [IN] Name of the component.
    const char*     name,           ///< [IN] Name of the pool inside the component.
    size_t      objSize ///< [IN] The size of the individual objects to be allocated from this pool
                        /// (in bytes).  E.g., sizeof(MyObject_t).
)
{
    le_mem_PoolRef_t newPool = malloc(sizeof(MemPool_t));

    // Crash if we can't create the memory pool.
    LE_ASSERT(newPool);

    // Initialize the memory pool.
    InitPool(newPool, componentName, name, objSize);

    Lock();

    // Generate an error if there are multiple pools with the same name.
    VerifyUniquenessOfName(newPool);

    // Add the new pool to the list of pools.
    PoolListChangeCount++;
    le_dls_Queue(&PoolList, &(newPool->poolLink));

    Unlock();

    return newPool;
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
    #ifndef LE_MEM_VALGRIND
        LE_ASSERT(pool);

        Lock();

        if (pool->superPoolPtr)
        {
            // This is a sub-pool so the memory blocks to create must come from the super-pool.
            // Check that there are enough blocks in the superpool.
            ssize_t numBlocksToAdd = numObjects - le_sls_NumLinks(&(pool->superPoolPtr->freeList));

            if (numBlocksToAdd > 0)
            {
                // Expand the super-pool.
                AddBlocks(pool->superPoolPtr, numBlocksToAdd);
            }

            // Move the blocks from the superpool to our pool.
            MoveBlocks(pool, pool->superPoolPtr, numObjects);

            // Update the sub-pool total block count.
            pool->totalBlocks = pool->totalBlocks + numObjects;

            // Update the super-pool's block use counts.
            pool->superPoolPtr->numBlocksInUse += numObjects;

            if (pool->superPoolPtr->numBlocksInUse > pool->superPoolPtr->maxNumBlocksUsed)
            {
                pool->superPoolPtr->maxNumBlocksUsed = pool->superPoolPtr->numBlocksInUse;
            }
        }
        else
        {
            // This is not a sub-pool.
            AddBlocks(pool, numObjects);
        }

        Unlock();
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

    Lock();

    #ifndef LE_MEM_VALGRIND
        // Pop a link off the pool.
        le_sls_Link_t* blockLinkPtr = le_sls_Pop(&(pool->freeList));

        if (blockLinkPtr != NULL)
        {
            // Get the block from the block link.
            blockPtr = CONTAINER_OF(blockLinkPtr, MemBlock_t, link);
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
        pool->numAllocations++;
        pool->numBlocksInUse++;

        if (pool->numBlocksInUse > pool->maxNumBlocksUsed)
        {
            pool->maxNumBlocksUsed = pool->numBlocksInUse;
        }

        blockPtr->refCount = 1;

        // Return the user object in the block.
        #ifdef USE_GUARD_BAND
            CheckGuardBands(blockPtr);
            userPtr = blockPtr->data + GUARD_BAND_SIZE;
        #else
            userPtr = blockPtr->data;
        #endif
    }

    Unlock();

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

    #ifndef LE_MEM_VALGRIND
        while ((objPtr = le_mem_TryAlloc(pool)) == NULL)
        {
            // Expand the pool.
            le_mem_ExpandPool(pool, pool->numBlocksToForce);

            Lock();
            pool->numOverflows++;

            // log a warning.
            LE_DEBUG("Memory pool '%s' overflowed. Expanded to %zu blocks.",
                    pool->name,
                    pool->totalBlocks);

            Unlock();
        }
    #else
        objPtr = le_mem_AssertAlloc(pool);
    #endif

    return objPtr;
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

    Lock();
    pool->numBlocksToForce = numObjects;
    Unlock();
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
    #ifdef USE_GUARD_BAND
        uint8_t* dataPtr = objPtr;
        dataPtr -= GUARD_BAND_SIZE;
        blockPtr = CONTAINER_OF(dataPtr, MemBlock_t, data);
    #else
        blockPtr = CONTAINER_OF(objPtr, MemBlock_t, data);
    #endif

    #ifdef USE_GUARD_BAND
        CheckGuardBands(blockPtr);
    #endif

    Lock();

    switch (blockPtr->refCount)
    {
        case 1:
        {
            MemPool_t* poolPtr = blockPtr->poolPtr;

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
                Unlock();
                destructor(objPtr);

                // Re-lock the mutex now so that it is safe to access the pool object again.
                Lock();
            }

            #ifndef LE_MEM_VALGRIND
                // Release the memory back into the pool.
                // Note that we don't do this before calling the destructor because the destructor
                // still needs to access it, but after it goes back on the free list, it could get
                // reallocated by another thread (or even the destructor itself) and have its
                // contents clobbered.
                le_sls_Stack(&(poolPtr->freeList), &(blockPtr->link));
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
                     blockPtr->poolPtr->name);

        default:
            blockPtr->refCount--;
    }

    Unlock();
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
    #ifdef USE_GUARD_BAND
        objPtr = (((uint8_t*)objPtr) - GUARD_BAND_SIZE);
    #endif
    MemBlock_t* memBlockPtr = CONTAINER_OF(objPtr, MemBlock_t, data);

    #ifdef USE_GUARD_BAND
        CheckGuardBands(memBlockPtr);
    #endif

    Lock();

    LE_ASSERT(memBlockPtr->refCount != 0);

    memBlockPtr->refCount++;

    Unlock();
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
    #ifdef USE_GUARD_BAND
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

    Lock();
    pool->destructor = destructor;
    Unlock();
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

    Lock();

    statsPtr->numAllocs = pool->numAllocations;
    statsPtr->numOverflows = pool->numOverflows;
    statsPtr->numFree = pool->totalBlocks - pool->numBlocksInUse;
    statsPtr->numBlocksInUse = pool->numBlocksInUse;
    statsPtr->maxNumBlocksUsed = pool->maxNumBlocksUsed;

    Unlock();
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

    Lock();
    pool->numAllocations = 0;
    pool->numOverflows = 0;
    Unlock();
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

    Lock();
    le_result_t result = le_utf8_Copy(namePtr, pool->name, bufSize, NULL);
    Unlock();

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
const bool le_mem_IsSubPool
(
    le_mem_PoolRef_t    pool        ///< [IN] The memory pool.
)
{
    LE_ASSERT(pool != NULL);

    bool isSubPool;

    Lock();
    isSubPool = (pool->superPoolPtr != NULL);
    Unlock();

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

    Lock();
    numBlocks = pool->totalBlocks;
    Unlock();

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

    Lock();
    objSize = pool->userDataSize;
    Unlock();

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

    Lock();
    objSize = pool->blockSize;
    Unlock();

    return objSize;
}


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

    Lock();

    // Search all pools except for the first one because the first pool is always the sub-pools pool.
    le_dls_Link_t* poolLinkPtr = le_dls_Peek(&PoolList);
    poolLinkPtr = le_dls_PeekNext(&PoolList, poolLinkPtr);

    while (poolLinkPtr)
    {
        MemPool_t* memPoolPtr = CONTAINER_OF(poolLinkPtr, MemPool_t, poolLink);

        if (strcmp(fullName, memPoolPtr->name) == 0)
        {
            result = memPoolPtr;
            break;
        }

        poolLinkPtr = le_dls_PeekNext(&PoolList, poolLinkPtr);
    }

    Unlock();

    return result;
}


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
le_mem_PoolRef_t _le_mem_CreateSubPool
(
    le_mem_PoolRef_t    superPool,  ///< [IN] The super-pool.
    const char*     componentName,  ///< [IN] Name of the component.
    const char*     name,           ///< [IN] Name of the pool inside the component.
    size_t              numObjects  ///< [IN] The number of objects to take from the super-pool.
)
{
    LE_ASSERT(superPool != NULL);

    // Make sure the parent pool is not itself a sub-pool.
    LE_ASSERT(superPool->superPoolPtr == NULL);

    // Get a sub-pool from the pool of sub-pools.
    le_mem_PoolRef_t subPool = le_mem_ForceAlloc(SubPoolsPool);

    // Initialize the pool.
    InitPool(subPool, componentName, name, superPool->userDataSize);
    subPool->superPoolPtr = superPool;

    Lock();

    // Log an error if the pool name is not unique.
    VerifyUniquenessOfName(subPool);

    // Add the sub-pool to the list of pools.
    PoolListChangeCount++;
    le_dls_Queue(&PoolList, &(subPool->poolLink));

    Unlock();

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

    Lock();

    // Make sure all sub-pool objects are free.
    le_mem_PoolRef_t superPool = subPool->superPoolPtr;

    LE_FATAL_IF(subPool->numBlocksInUse != 0,
                "Subpool '%s' deleted while %zu blocks remain allocated.",
                subPool->name,
                subPool->numBlocksInUse);

    size_t numBlocks = subPool->totalBlocks;

    // Move the blocks from the subPool back to the superpool.
    MoveBlocks(superPool, subPool, numBlocks);

    // Update the superPool's block use count.
    superPool->numBlocksInUse -= numBlocks;

    // Remove the sub-pool from the list of sub-pools.
    PoolListChangeCount++;
    le_dls_Remove(&PoolList, &(subPool->poolLink));

    Unlock();

    // Release the sub-pool.
    le_mem_Release(subPool);
}


