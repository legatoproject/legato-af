/**
 * @page c_memory Dynamic Memory Allocation API
 *
 *
 * @subpage le_mem.h "API Reference"
 *
 * <HR>
 *
 * Dynamic memory allocation (especially deallocation) using the C runtime heap, through
 * malloc, free, strdup, calloc, realloc, etc. can result in performance degradation and out-of-memory
 * conditions.
 *
 * This is due to fragmentation of the heap. The degraded performance and exhausted memory result from indirect interactions
 * within the heap between unrelated application code. These issues are non-deterministic,
 * and can be very difficult to rectify.
 *
 * Memory Pools offer a powerful solution.  They trade-off a deterministic amount of
 * memory for
 *  - deterministic behaviour,
 *  - O(1) allocation and release performance, and
 *  - built-in memory allocation tracking.
 *
 * And it brings the power of @b destructors to C!
 *
 * @todo
 *     Do we need to add a "resettable heap" option to the memory management tool kit?
 *
 *
 * @section mem_overview Overview
 *
 * The most basic usage involves:
 *  - Creating a pool (usually done once at process start-up)
 *  - Allocating objects (memory blocks) from a pool
 *  - Releasing objects back to their pool.
 *
 * Pools generally can't be deleted.  You create them when your process
 * starts-up, and use them until your process terminates. It's up to the OS to clean-up the memory
 * pools, along with everything else your process is using, when your process terminates.  (Although,
 * if you find yourself really needing to delete pools, @ref mem_sub_pools could offer you a solution.)
 *
 * Pools also support the following advanced features:
 *  - reference counting
 *  - destructors
 *  - statistics
 *  - multi-threading
 *  - sub-pools (pools that can be deleted).
 *
 * The following sections describe these, beginning with the most basic usage and working up to more
 * advanced topics.
 *
 *
 * @section mem_creating Creating a Pool
 *
 * Before allocating memory from a pool, the pool must be created using le_mem_CreatePool(), passing
 * it the name of the pool and the size of the objects to be allocated from that pool. This returns
 * a reference to the new pool, which has zero free objects in it.
 *
 * To populate your new pool with free objects, you call @c le_mem_ExpandPool().
 * This is separated into two functions (rather than having
 * one function with three parameters) to make it virtually impossible to accidentally get the parameters
 * in the wrong order (which would result in nasty bugs that couldn't be caught by the compiler).
 * The ability to expand pools comes in handy (see @ref mem_pool_sizes).
 *
 * This code sample defines a class "Point" and a pool "PointPool" used to
 * allocate memory for objects of that class:
 * @code
 * #define MAX_POINTS 12  // Maximum number of points that can be handled.
 *
 * typedef struct
 * {
 *     int x;  // pixel position along x-axis
 *     int y;  // pixel position along y-axis
 * }
 * Point_t;
 *
 * le_mem_PoolRef_t PointPool;
 *
 * int xx_pt_ProcessStart(void)
 * {
 *     PointPool = le_mem_CreatePool("xx.pt.Points", sizeof(Point_t));
 *     le_mem_ExpandPool(PointPool, MAX_POINTS);
 *
 *     return SUCCESS;
 * }
 * @endcode
 *
 * To make things easier for power-users, @c le_mem_ExpandPool() returns the same pool
 * reference that it was given. This allows the xx_pt_ProcessStart() function to be re-implemented
 * as follows:
 * @code
 * int xx_pt_ProcessStart(void)
 * {
 *     PointPool = le_mem_ExpandPool(le_mem_CreatePool(sizeof(Point_t)), MAX_POINTS);
 *
 *     return SUCCESS;
 * }
 * @endcode
 *
 * Although this requires a dozen or so fewer keystrokes of typing and occupies one less line
 * of code, it's arguably less readable than the previous example.
 *
 * For a discussion on how to pick the number of objects to have in your pools, see @ref mem_pool_sizes.
 *
 * @section mem_allocating Allocating From a Pool
 *
 * Allocating from a pool has multiple options:
 *  - @c le_mem_TryAlloc() - Quietly return NULL if there are no free blocks in the pool.
 *  - @c le_mem_AssertAlloc() - Log an error and take down the process if there are no free blocks in
 *                         the pool.
 *  - @c le_mem_ForceAlloc() - If there are no free blocks in the pool, log a warning and automatically
 *                         expand the pool (or log an error and terminate the calling process there's
 *                         not enough free memory to expand the pool).
 *
 * All of these functions take a pool reference and return a pointer to the object
 * allocated from the pool.
 *
 * The first option, using @c le_mem_TryAlloc(), is the
 *  closest to the way good old malloc() works.  It requires the caller check the
 * return code to see if it's NULL.  This can be annoying enough that a lot of
 * people get lazy and don't check the return code (Bad programmer! Bad!). It turns out that
 * this option isn't really what people usually want (but occasionally they do)
 *
 * The second option, using @c le_mem_AssertAlloc(), is only used when the allocation should
 * never fail, by design; a failure to allocate a block is a fatal error.
 * This isn't often used, but can save a lot of boilerplate error checking code.
 *
 * The third option, @c using le_mem_ForceAlloc(), is the one that gets used most often.
 * It allows developers to avoid writing error checking code, because the allocation will
 * essentially never fail because it's handled inside the memory allocator.  It also
 * allows developers to defer fine tuning their pool sizes until after they get things working.
 * Later, they check the logs for pool size usage, and then modify their pool sizes accordingly.
 * If a particular pool is continually growing, it's a good indication there's a
 * memory leak. This permits seeing exactly what objects are being leaked.  If certain debug
 * options are turned on, they can even find out which line in which file allocated the blocks
 *  being leaked.
 *
 *
 * @section mem_releasing Releasing Back Into a Pool
 *
 * Releasing memory back to a pool never fails, so there's no need to check a return code.
 * Also, each object knows which pool it came from, so the code that releases the object doesn't have to care.
 * All it has to do is call @c le_mem_Release() and pass a pointer to the object to be released.
 *
 * The critical thing to remember is that once an object has been released, it
 * <b> must never be accessed again </b>.  Here is a <b> very bad code example</b>:
 * @code
 *     Point_t* pointPtr = le_mem_ForceAlloc(PointPool);
 *     pointPtr->x = 5;
 *     pointPtr->y = 10;
 *     le_mem_Release(pointPtr);
 *     printf("Point is at position (%d, %d).\n", pointPtr->x, pointPtr->y);
 * @endcode
 *
 *
 * @section mem_ref_counting Reference Counting
 *
 * Reference counting is a powerful feature of our memory pools.  Here's how it works:
 *  - Every object allocated from a pool starts with a reference count of 1.
 *  - Whenever someone calls le_mem_AddRef() on an object, its reference count is incremented by 1.
 *  - When it's released, its reference count is decremented by 1.
 *  - When its reference count reaches zero, it's destroyed (i.e., its memory is released back into the pool.)
 *
 * This allows one function to:
 *  - create an object.
 *  - work with it.
 *  - increment its reference count and pass a pointer to the object to another function (or thread, data structure, etc.).
 *  - work with it some more.
 *  - release the object without having to worry about when the other function is finished with it.
 *
 * The other function also releases the object when it's done with it.  So, the object will
 * exist until both functions are done.
 *
 * If there are multiple threads involved, be careful to protect the shared
 * object from race conditions(see the @ref mem_threading).
 *
 * Another great advantage of reference counting is it enables @ref mem_destructors.
 *
 * @note le_mem_GetRefCount() can be used to check the current reference count on an object.
 *
 * @section mem_destructors Destructors
 *
 * Destructors are a powerful feature of C++.  Anyone who has any non-trivial experience with C++ has
 * used them.  Because C was created before object-oriented programming was around, there's
 * no native language support for destructors in C.  Object-oriented design is still possible
 * and highly desireable even when the programming is done in C.
 *
 * In Legato, it's possible to call @c le_mem_SetDestructor() to attach a function to a memory pool
 * to be used as a destructor for objects allocated from that pool. If a pool has a destructor,
 *  whenever the reference count reaches zero for an object allocated from that pool,
 * the pool's destructor function will pass a pointer to that object.  After
 * the destructor returns, the object will be fully destroyed, and its memory will be released back
 * into the pool for later reuse by another object.
 *
 * Here's a destructor code sample:
 * @code
 * static void PointDestructor(void* objPtr)
 * {
 *     Point_t* pointPtr = objPtr;
 *
 *     printf("Destroying point (%d, %d)\n", pointPtr->x, pointPtr->y);
 *
 *     @todo Add more to sample.
 * }
 *
 * int xx_pt_ProcessStart(void)
 * {
 *     PointPool = le_mem_CreatePool(sizeof(Point_t));
 *     le_mem_ExpandPool(PointPool, MAX_POINTS);
 *     le_mem_SetDestructor(PointPool, PointDestructor);
 *     return SUCCESS;
 * }
 *
 * static void DeletePointList(Point_t** pointList, size_t numPoints)
 * {
 *     size_t i;
 *     for (i = 0; i < numPoints; i++)
 *     {
 *         le_mem_Release(pointList[i]);
 *     }
 * }
 * @endcode
 *
 * In this sample, when DeletePointList() is called (with a pointer to an array of pointers
 * to Point_t objects with reference counts of 1), each of the objects in the pointList is
 * released.  This causes their reference counts to hit 0, which triggers executing
 * PointDestructor() for each object in the pointList, and the "Destroying point..." message will
 * be printed for each.
 *
 *
 * @section mem_stats Statistics
 *
 * Some statistics are gathered for each memory pool:
 *  - Number of allocations.
 *  - Number of currently free objects.
 *  - Number of overflows (times that le_mem_ForceAlloc() had to expand the pool).
 *
 * Statistics (and other pool properties) can be checked using functions:
 *  - @c le_mem_GetStats()
 *  - @c le_mem_GetObjectCount()
 *  - @c le_mem_GetObjectSize()
 *
 * Statistics are fetched together atomically using a single function call.
 * This prevents inconsistencies between them if in a multi-threaded program.
 *
 * If you don't have a reference to a specified pool, but you have the name of the pool, you can
 * get a reference to the pool using @c le_mem_FindPool().
 *
 * In addition to programmatically fetching these, they're also available through the
 * "poolstat" console command (unless your process's main thread is blocked).
 *
 * To reset the pool statistics, use @c le_mem_ResetStats().
 *
 * @section mem_diagnostics Diagnostics
 *
 * The memory system also supports two different forms of diagnostics.  Both are enabled by setting
 * the appropriate KConfig options when building the framework.
 *
 * The first of these options is @ref MEM_TRACE.  When you enable @ref MEM_TRACE every pool is given
 * a tracepoint with the name of the pool on creation.
 *
 * For instance, the configTree node pool is called, "configTree.nodePool".  So to enable a trace of
 * all config tree node creation and deletion one would use the log tool as follows:
 *
 * @code
 * $ log trace configTree.nodePool
 * @endcode
 *
 * The second diagnostic build flag is @ref MEM_POOLS.  When @ref MEM_POOLS is disabled, the pools
 * are disabled and instead malloc and free are directly used.  Thus enabling the use of tools
 * like Valgrind.
 *
 * @section mem_threading Multi-Threading
 *
 * All functions in this API are <b> thread-safe, but not async-safe </b>.  The objects
 * allocated from pools are not inherently protected from races between threads.
 *
 * Allocating and releasing objects, checking stats, incrementing reference
 * counts, etc. can all be done from multiple threads (excluding signal handlers) without having
 * to worry about corrupting the memory pools' hidden internal data structures.
 *
 * There's no magical way to prevent different threads from interferring with each other
 * if they both access the @a contents of the same object at the same time.
 *
 * The best way to prevent multi-threaded race conditions is simply don't share data between
 * threads. If multiple threads must access the same data structure, then mutexes, semaphores,
 * or similar methods should be used to @a synchronize threads and avoid
 * data structure corruption or thread misbehaviour.
 *
 * Although memory pools are @a thread-safe, they are not @a async-safe. This means that memory pools
 * @a can be corrupted if they are accessed by a signal handler while they are being accessed
 * by a normal thread.  To be safe, <b> don't call any memory pool functions from within a signal handler. </b>
 *
 * One problem using destructor functions in a
 * multi-threaded environment is that the destructor function modifies a data structure shared
 * between threads, so it's easy to forget to synchronize calls to @c le_mem_Release() with other code
 *  accessing the data structure.  If a mutex is used to coordinate access to
 * the data structure, then the mutex must be held by the thread that calls le_mem_Release() to
 * ensure there's no other thread accessing the data structure when the destructor runs.
 *
 * @section mem_pool_sizes Managing Pool Sizes
 *
 * We know it's possible to have pools automatically expand
 * when they are exhausted, but we don't really want that to happen normally.
 * Ideally, the pools should be fully allocated to their maximum sizes at start-up so there aren't
 * any surprises later when certain feature combinations cause the
 * system to run out of memory in the field.  If we allocate everything we think
 * is needed up-front, then we are much more likely to uncover any memory shortages during
 * testing, before it's in the field.
 *
 * Choosing the right size for your pools correctly at start-up is easy to do if there is a maximum
 * number of fixed, external @a things that are being represented by the objects being allocated
 * from the pool.  If the pool holds "call objects" representing phone calls over a T1
 * carrier that will never carry more than 24 calls at a time, then it's obvious that you need to
 * size your call object pool at 24.
 *
 * Other times, it's not so easy to choose the pool size like
 * code to be reused in different products or different configurations that have different
 * needs.  In those cases, you still have a few options:
 *
 *  - At start-up, query the operating environment and base the pool sizes.
 *  - Read a configuration setting from a file or other configuration data source.
 *  - Use a build-time configuration setting.
 *
 * The build-time configuration setting is the easiest, and generally requires less
 * interaction between components at start-up simplifying APIs
 * and reducing boot times.
 *
 * If the pool size must be determined at start-up, use @c le_mem_ExpandPool().
 * Perhaps there's a service-provider module designed to allocate objects on behalf
 * of client. It can have multiple clients at the same time, but it doesn't know how many clients
 * or what their resource needs will be until the clients register with it at start-up.  We'd want
 * those clients to be as decoupled from each other as possible (i.e., we want the clients know as little
 * as possible about each other); we don't want the clients to get together and add up all
 * their needs before telling the service-provider.  We'd rather have the clients independently
 * report their own needs to the service-provider.  Also, we don't want each client to have to wait
 * for all the other clients to report their needs before starting to use
 * the services offered by the service-provider.  That would add more complexity to the interactions
 * between the clients and the service-provider.
 *
 * This is what should happen when the service-provider can't wait for all clients
 * to report their needs before creating the pool:
 *  - When the service-provider starts up, it creates an empty pool.
 *  - Whenever a client registers itself with the service-provider, the client can tell the
 *    service-provider what its specific needs are, and the service-provider can expand its
 *    object pool accordingly.
 *  - Since registrations happen at start-up, pool expansion occurs
 *    at start-up, and testing will likely find any pool sizing before going into the field.
 *
 * Where clients dynamically start and stop during runtime in response
 * to external events (e.g., when someone is using the device's Web UI), we still have
 * a problem because we can't @a shrink pools or delete pools when clients go away.  This is where
 * @ref mem_sub_pools is useful.
 *
 * @section mem_sub_pools Sub-Pools
 *
 * Essentially, a Sub-Pool is a memory pool that gets its blocks from another pool (the super-pool).
 * Sub Pools @a can be deleted, causing its blocks to be released back into the super-pool.
 *
 * This is useful when a service-provider module needs to handle clients that
 * dynamically pop into existence and later disappear again.  When a client attaches to the service
 * and says it will probably need a maximum of X of the service-provider's resources, the
 * service provider can set aside that many of those resources in a sub-pool for that client.
 * If that client goes over its limit, the sub-pool will log a warning message.
 *
 * The problem of sizing the super-pool correctly at start-up still exists,
 * so what's the point of having a sub-pool, when all of the resources could just be allocated from
 * the super-pool?
 *
 * The benefit is really gained in troubleshooting.  If client A, B, C, D and E are
 * all behaving nicely, but client F is leaking resources, the sub-pool created
 * on behalf of client F will start warning about the memory leak; time won't have to be
 * wasted looking at clients A through E to rule them out.
 *
 * To create a sub-pool, call @c le_mem_CreateSubPool(). It takes a reference to the super-pool
 * and the number of objects to move to the sub-pool, and it returns a reference to the new sub-pool.
 *
 * To delete a sub-pool, call @c le_mem_DeleteSubPool().  Do not try to use it to delete a pool that
 * was created using le_mem_CreatePool().  It's only for sub-pools created using le_mem_CreateSubPool().
 * Also, it's @b not okay to delete a sub-pool while there are still blocks allocated from it, or
 * if it has any sub-pools.  You'll see errors in your logs if you do that.
 *
 * Sub-Pools automatically inherit their parent's destructor function.
 *
 * @section mem_reduced_pools Reduced-size pools
 *
 * One problem that occurs with memory pools is where objects of different sizes need to be
 * stored.  A classic example is strings -- the longest string an application needs to be able
 * to handle may be much longer than the typical string size.  In this case a lot of memory
 * will be wasted with standard memory pools, since all objects allocated from the pool will
 * be the size of the longest possible object.
 *
 * The solution is to use reduced-size pools.  These are a kind of sub-pool where the size
 * of the object in the sub-pool is different from the size of the object in the super-pool.
 * This way multiple blocks from the sub-pool can be stored in a single block of the super-pool.
 *
 * Reduced-size pools have some limitations:
 *
 *  - Each block in the super-pool is divided up to create blocks in the subpool.  So subpool
 *    blocks sizes must be less than half the size of the super-pool block size.  An attempt to
 *    create a pool with a larger block size will just return a reference to the super-pool.
 *  - Due overhead, each block actually requires 8-80 bytes more space than requested.
 *    There is little point in subdividing pools < 4x overhead, or ~300 bytes in the default
 *    configuration.  Note the exact amount of overhead depends on the size of the guard bands
 *    and the size of pointer on your target.
 *  - Blocks used by the reduced pool are permanently moved from the super-pool to the reduced
 *    pool.  This must be taken into account when sizing the super-pool.
 *
 * Example 1:
 *
 * A network service needs to allocate space for packets received over a network.  It should
 * Typical packet length is up to 200 bytes, but occasional packets may be up to 1500 bytes.  The
 * service needs to be able to queue at least 32 packets, up to 5 of which can be 1500 bytes.
 * Two pools are used: A pool of 12 objects 1500 bytes in size, and a reduced-size pool of 32
 * objects 200 bytes in size (280 bytes with overhead).  Seven objects from the superpool are
 * used to store reduced-size objects.  This represents a memory savings of 60% compared with
 * a pool of 32 1500 byte objects.
 *
 * Example 2:
 *
 * An application builds file paths which can be from 16-70 bytes.  Only three such paths can be
 * created at a time, but they can be any size.  In this case it is better to just create
 * a single pool of three 70-byte objects.  Most of the potential space gains would be consumed
 * by overhead.  Even if this was not the case, the super pool still needs three free objects
 * in addition to the objects required by the subpool, so there is no space savings.
 *
 * To create a reduced-size pool, use @c le_mem_CreateReducedPool().  It takes a reference to the
 * super-pool, the initial number of objects in the sub-pool, and size of an object in the sub-pool
 * compared with the parent pool, and it returns a reference to the new sub-pool.
 *
 * Reduced-size pools are deleted using @c le_mem_DeleteSubPool() like other sub-pools.
 *
 * To help the programmer pick the right pool to allocate from, reduced-size pools provide
 * @c le_mem_TryVarAlloc(), @c le_mem_AssertVarAlloc() and @c le_mem_ForceVarAlloc() functions.
 * In addition to the pool these take the object size to allocate.  If this size is larger than
 * the pool's object size and the pool is a reduced-size pool, and there is a parent pool
 * large enough for this object, it will allocate the object from the parent pool instead.
 * If no parent pool is large enough, the program will exit.
 *
 * You can call @c le_mem_GetBlockSize() to get the actual size of the object returned by
 * one of these functions.
 *
 * As with other sub-pools, they cannot be deleted while any blocks are allocated from the pool,
 * or it has any sub-pools.  Reduced-size pools also automatically inherit their parent's
 * destructor function.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/** @file le_mem.h
 *
 * Legato @ref c_memory include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MEM_INCLUDE_GUARD
#define LEGATO_MEM_INCLUDE_GUARD

#ifndef LE_COMPONENT_NAME
#   define LE_COMPONENT_NAME
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for destructor functions.
 *
 * @param objPtr   Pointer to the object where reference count has reached zero.  After the destructor
 *                 returns this object's memory will be released back into the pool (and this pointer
 *                 will become invalid).
 *
 * @return Nothing.
 *
 * See @ref mem_destructors for more information.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_mem_Destructor_t)
(
    void* objPtr    ///< see parameter documentation in comment above.
);

// Max memory pool name bytes -- must match definition in limit.h
#define LE_MEM_LIMIT_MAX_MEM_POOL_NAME_BYTES 32

//--------------------------------------------------------------------------------------------------
/**
 * Definition of a memory pool.
 *
 * @note This should not be used directly.  To create a memory pool use either le_mem_CreatePool()
 *       or LE_MEM_DEFINE_STATIC_POOL()/le_mem_InitStaticPool().
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mem_Pool
{
    le_dls_Link_t poolLink;             ///< This pool's link in the list of memory pools.
    struct le_mem_Pool* superPoolPtr;   ///< A pointer to our super pool if we are a sub-pool. NULL
                                        ///  if we are not a sub-pool.
#if LE_CONFIG_MEM_POOL_STATS
    // These members go before LE_CONFIG_MEM_POOLS so numAllocations will always be aligned, even on
    // 32-bit architectures, even when LE_CONFIG_MEM_POOLS is not declared
    size_t numOverflows;                ///< Number of times le_mem_ForceAlloc() had to expand pool.
    uint64_t numAllocations;            ///< Total number of times an object has been allocated
                                        ///  from this pool.
    size_t maxNumBlocksUsed;            ///< Maximum number of allocated blocks at any one time.
#endif
#if LE_CONFIG_MEM_POOLS
    le_sls_List_t freeList;             ///< List of free memory blocks.
#endif

    size_t userDataSize;                ///< Size of the object requested by the client in bytes.
    size_t blockSize;                   ///< Number of bytes in a block, including all overhead.
    size_t totalBlocks;                 ///< Total number of blocks in this pool including free
                                        ///  and allocated blocks.
    size_t numBlocksInUse;              ///< Number of currently allocated blocks.
    size_t numBlocksToForce;            ///< Number of blocks that is added when Force Alloc
                                        ///  expands the pool.
#if LE_CONFIG_MEM_TRACE
    le_log_TraceRef_t memTrace;         ///< If tracing is enabled, keeps track of a trace object
                                        ///< for this pool.
#endif

    le_mem_Destructor_t destructor;     ///< The destructor for objects in this pool.
#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
    char name[LE_MEM_LIMIT_MAX_MEM_POOL_NAME_BYTES]; ///< Name of the pool.
#endif
}
le_mem_Pool_t;


//--------------------------------------------------------------------------------------------------
/**
 * Objects of this type are used to refer to a memory pool created using either
 * le_mem_CreatePool() or le_mem_CreateSubPool().
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mem_Pool* le_mem_PoolRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * List of memory pool statistics.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    size_t      numBlocksInUse;     ///< Number of currently allocated blocks.
    size_t      maxNumBlocksUsed;   ///< Maximum number of allocated blocks at any one time.
    size_t      numOverflows;       ///< Number of times le_mem_ForceAlloc() had to expand the pool.
    uint64_t    numAllocs;          ///< Number of times an object has been allocated from this pool.
    size_t      numFree;            ///< Number of free objects currently available in this pool.
}
le_mem_PoolStats_t;


#if LE_CONFIG_MEM_TRACE
    //----------------------------------------------------------------------------------------------
    /**
     * Internal function used to retrieve a pool handle for a given pool block.
     */
    //----------------------------------------------------------------------------------------------
    le_mem_PoolRef_t _le_mem_GetBlockPool
    (
        void*   objPtr  ///< [IN] Pointer to the object we're finding a pool for.
    );

    typedef void* (*_le_mem_AllocFunc_t)(le_mem_PoolRef_t pool);

    //----------------------------------------------------------------------------------------------
    /**
     * Internal function used to call a memory allocation function and trace its call site.
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
    );

    //----------------------------------------------------------------------------------------------
    /**
     * Internal function used to call a variable memory allocation function and trace its call site.
     */
    //----------------------------------------------------------------------------------------------
    void* _le_mem_VarAllocTracer
    (
        le_mem_PoolRef_t     pool,             ///< [IN] The pool activity we're tracing.
        size_t               size,             ///< [IN] The size of block to allocate
        _le_mem_AllocFunc_t  funcPtr,          ///< [IN] Pointer to the mem function in question.
        const char*          poolFunction,     ///< [IN] The pool function being called.
        const char*          file,             ///< [IN] The file the call came from.
        const char*          callingfunction,  ///< [IN] The function calling into the pool.
        size_t               line              ///< [IN] The line in the function where the call
                                               ///       occurred.
    );

    //----------------------------------------------------------------------------------------------
    /**
     * Internal function used to trace memory pool activity.
     */
    //----------------------------------------------------------------------------------------------
    void _le_mem_Trace
    (
        le_mem_PoolRef_t    pool,             ///< [IN] The pool activity we're tracing.
        const char*         file,             ///< [IN] The file the call came from.
        const char*         callingfunction,  ///< [IN] The function calling into the pool.
        size_t              line,             ///< [IN] The line in the function where the call
                                              ///       occurred.
        const char*         poolFunction,     ///< [IN] The pool function being called.
        void*               blockPtr          ///< [IN] Block allocated/freed.
    );
#endif


/// @cond HIDDEN_IN_USER_DOCS
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
    size_t      objSize, ///< [IN] Size of the individual objects to be allocated from this pool
                        /// (in bytes), e.g., sizeof(MyObject_t).
    le_mem_Pool_t* poolPtr, ///< [IN] Pointer to pre-allocated pool header.
    void* poolDataPtr       ///< [IN] Pointer to pre-allocated pool data.
);
/// @endcond


#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/** @cond HIDDEN_IN_USER_DOCS
 *
 * Internal function used to implement le_mem_CreatePool() with automatic component scoping
 * of pool names.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t _le_mem_CreatePool
(
    const char* componentName,  ///< [IN] Name of the component.
    const char* name,           ///< [IN] Name of the pool inside the component.
    size_t      objSize         ///< [IN] Size of the individual objects to be allocated from this pool
                                /// (in bytes), e.g., sizeof(MyObject_t).
);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Creates an empty memory pool.
 *
 * @return
 *      Reference to the memory pool object.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
static inline le_mem_PoolRef_t le_mem_CreatePool
(
    const char* name,           ///< [IN] Name of the pool inside the component.
    size_t      objSize         ///< [IN] Size of the individual objects to be allocated from this pool
                                /// (in bytes), e.g., sizeof(MyObject_t).
)
{
    return _le_mem_CreatePool(STRINGIZE(LE_COMPONENT_NAME), name, objSize);
}
#else /* if not LE_CONFIG_MEM_POOL_NAMES_ENABLED */
//--------------------------------------------------------------------------------------------------
/** @cond HIDDEN_IN_USER_DOCS
 *
 * Internal function used to implement le_mem_CreatePool() with automatic component scoping
 * of pool names.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t _le_mem_CreatePool
(
    size_t      objSize         ///< [IN] Size of the individual objects to be allocated from this pool
                                /// (in bytes), e.g., sizeof(MyObject_t).
);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Creates an empty memory pool.
 *
 * @return
 *      Reference to the memory pool object.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE le_mem_PoolRef_t le_mem_CreatePool
(
    const char* name,           ///< [IN] Name of the pool inside the component.
    size_t      objSize         ///< [IN] Size of the individual objects to be allocated from this pool
                                /// (in bytes), e.g., sizeof(MyObject_t).
)
{
    return _le_mem_CreatePool(objSize);
}
#endif /* end LE_CONFIG_MEM_POOL_NAMES_ENABLED */

//--------------------------------------------------------------------------------------------------
/**
 * Number of words in a memory pool, given number of blocks and object size.
 *
 * @note Only used internally
 */
//--------------------------------------------------------------------------------------------------
#define LE_MEM_POOL_WORDS(numBlocks, objSize)                           \
    ((numBlocks)*(((sizeof(le_mem_Pool_t*) + sizeof(size_t)) /* sizeof(MemBlock_t) */ + \
                   (((objSize)<sizeof(le_sls_Link_t))?sizeof(le_sls_Link_t):(objSize))+ \
                   sizeof(uint32_t)*LE_CONFIG_NUM_GUARD_BAND_WORDS*2+      \
                   sizeof(size_t) - 1) / sizeof(size_t)))

//--------------------------------------------------------------------------------------------------
/**
 * Declare variables for a static memory pool.
 *
 * In a static memory pool initial pool memory is statically allocated at compile time, ensuring
 * pool can be created with at least some elements.  This is especially valuable on embedded
 * systems.
 */
/*
 * Internal Note: size_t is used instead of uint8_t to ensure alignment on platforms where
 * alignment matters.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_MEM_POOLS
#  define LE_MEM_DEFINE_STATIC_POOL(name, numBlocks, objSize)             \
    static le_mem_Pool_t _mem_##name##Pool;                             \
    static size_t _mem_##name##Data[LE_MEM_POOL_WORDS(numBlocks, objSize)]
#else
#  define LE_MEM_DEFINE_STATIC_POOL(name, numBlocks, objSize)             \
    static le_mem_Pool_t _mem_##name##Pool
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Initialize an empty static memory pool.
 *
 * @return
 *      Reference to the memory pool object.
 *
 * @note
 *      This function cannot fail.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_MEM_POOLS
#  define le_mem_InitStaticPool(name, numBlocks, objSize)                                   \
    (inline_static_assert(                                                                  \
        sizeof(_mem_##name##Data) == sizeof(size_t[LE_MEM_POOL_WORDS(numBlocks, objSize)]), \
        "initial pool size does not match definition"),                                     \
    _le_mem_InitStaticPool(STRINGIZE(LE_COMPONENT_NAME), #name, (numBlocks), (objSize),     \
                           &_mem_##name##Pool, _mem_##name##Data))
#else
#  define le_mem_InitStaticPool(name, numBlocks, objSize)                               \
    _le_mem_InitStaticPool(STRINGIZE(LE_COMPONENT_NAME), #name, (numBlocks), (objSize), \
                           &_mem_##name##Pool, NULL)
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Expands the size of a memory pool.
 *
 * @return  Reference to the memory pool object (the same value passed into it).
 *
 * @note    On failure, the process exits, so you don't have to worry about checking the returned
 *          reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t le_mem_ExpandPool
(
    le_mem_PoolRef_t    pool,       ///< [IN] Pool to be expanded.
    size_t              numObjects  ///< [IN] Number of objects to add to the pool.
);



#if !LE_CONFIG_MEM_TRACE
    //----------------------------------------------------------------------------------------------
    /**
     * Attempts to allocate an object from a pool.
     *
     * @return
     *      Pointer to the allocated object, or NULL if the pool doesn't have any free objects
     *      to allocate.
     */
    //----------------------------------------------------------------------------------------------
    void* le_mem_TryAlloc
    (
        le_mem_PoolRef_t    pool    ///< [IN] Pool from which the object is to be allocated.
    );
#else
    /// @cond HIDDEN_IN_USER_DOCS
    void* _le_mem_TryAlloc(le_mem_PoolRef_t pool);
    /// @endcond

#   define le_mem_TryAlloc(pool)                                                                    \
        _le_mem_AllocTracer(pool,                                                                   \
                            _le_mem_TryAlloc,                                                       \
                            "le_mem_TryAlloc",                                                      \
                            STRINGIZE(LE_FILENAME),                                                 \
                            __FUNCTION__,                                                           \
                            __LINE__)

#endif


#if !LE_CONFIG_MEM_TRACE
    //----------------------------------------------------------------------------------------------
    /**
     * Allocates an object from a pool or logs a fatal error and terminates the process if the pool
     * doesn't have any free objects to allocate.
     *
     * @return Pointer to the allocated object.
     *
     * @note    On failure, the process exits, so you don't have to worry about checking the
     *          returned pointer for validity.
     */
    //----------------------------------------------------------------------------------------------
    void* le_mem_AssertAlloc
    (
        le_mem_PoolRef_t    pool    ///< [IN] Pool from which the object is to be allocated.
    );
#else
    /// @cond HIDDEN_IN_USER_DOCS
    void* _le_mem_AssertAlloc(le_mem_PoolRef_t pool);
    /// @endcond

#   define le_mem_AssertAlloc(pool)                                                                 \
        _le_mem_AllocTracer(pool,                                                                   \
                            _le_mem_AssertAlloc,                                                    \
                            "le_mem_AssertAlloc",                                                   \
                            STRINGIZE(LE_FILENAME),                                                 \
                            __FUNCTION__,                                                           \
                            __LINE__)
#endif


#if !LE_CONFIG_MEM_TRACE
    //----------------------------------------------------------------------------------------------
    /**
     * Allocates an object from a pool or logs a warning and expands the pool if the pool
     * doesn't have any free objects to allocate.
     *
     * @return  Pointer to the allocated object.
     *
     * @note    On failure, the process exits, so you don't have to worry about checking the
     *          returned pointer for validity.
     */
    //----------------------------------------------------------------------------------------------
    void* le_mem_ForceAlloc
    (
        le_mem_PoolRef_t    pool    ///< [IN] Pool from which the object is to be allocated.
    );
#else
    /// @cond HIDDEN_IN_USER_DOCS
    void* _le_mem_ForceAlloc(le_mem_PoolRef_t pool);
    /// @endcond

#   define le_mem_ForceAlloc(pool)                                                                  \
        _le_mem_AllocTracer(pool,                                                                   \
                            _le_mem_ForceAlloc,                                                     \
                            "le_mem_ForceAlloc",                                                    \
                            STRINGIZE(LE_FILENAME),                                                 \
                            __FUNCTION__,                                                           \
                            __LINE__)
#endif


#if !LE_CONFIG_MEM_TRACE
    //----------------------------------------------------------------------------------------------
    /**
     * Attempts to allocate an object from a pool.
     *
     * @return
     *      Pointer to the allocated object, or NULL if the pool doesn't have any free objects
     *      to allocate.
     */
    //----------------------------------------------------------------------------------------------
    void* le_mem_TryVarAlloc
    (
        le_mem_PoolRef_t    pool,         ///< [IN] Pool from which the object is to be allocated.
        size_t              size          ///< [IN] The size of block to allocate
    );
#else
    /// @cond HIDDEN_IN_USER_DOCS
    void* _le_mem_TryVarAlloc(le_mem_PoolRef_t pool, size_t size);
    /// @endcond

#   define le_mem_TryVarAlloc(pool, size)                               \
         _le_mem_VarAllocTracer(pool,                                   \
                                size,                                   \
                                _le_mem_TryVarAlloc,                    \
                                "le_mem_TryVarAlloc",                   \
                                STRINGIZE(LE_FILENAME),                 \
                                __FUNCTION__,                           \
                                __LINE__)

#endif


#if !LE_CONFIG_MEM_TRACE
    //----------------------------------------------------------------------------------------------
    /**
     * Allocates an object from a pool or logs a fatal error and terminates the process if the pool
     * doesn't have any free objects to allocate.
     *
     * @return Pointer to the allocated object.
     *
     * @note    On failure, the process exits, so you don't have to worry about checking the
     *          returned pointer for validity.
     */
    //----------------------------------------------------------------------------------------------
    void* le_mem_AssertVarAlloc
    (
        le_mem_PoolRef_t    pool,        ///< [IN] Pool from which the object is to be allocated.
        size_t              size         ///< [IN] The size of block to allocate
    );
#else
    /// @cond HIDDEN_IN_USER_DOCS
    void* _le_mem_AssertVarAlloc(le_mem_PoolRef_t pool, size_t size);
    /// @endcond

#   define le_mem_AssertVarAlloc(pool, size)                            \
        _le_mem_VarAllocTracer(pool,                                    \
                               size,                                    \
                               _le_mem_AssertVarAlloc,                  \
                               "le_mem_AssertVarAlloc",                 \
                               STRINGIZE(LE_FILENAME),                  \
                               __FUNCTION__,                            \
                               __LINE__)
#endif


#if !LE_CONFIG_MEM_TRACE
    //----------------------------------------------------------------------------------------------
    /**
     * Allocates an object from a pool or logs a warning and expands the pool if the pool
     * doesn't have any free objects to allocate.
     *
     * @return  Pointer to the allocated object.
     *
     * @note    On failure, the process exits, so you don't have to worry about checking the
     *          returned pointer for validity.
     */
    //----------------------------------------------------------------------------------------------
    void* le_mem_ForceVarAlloc
    (
        le_mem_PoolRef_t    pool,        ///< [IN] Pool from which the object is to be allocated.
        size_t              size         ///< [IN] The size of block to allocate
    );
#else
    /// @cond HIDDEN_IN_USER_DOCS
    void* _le_mem_ForceVarAlloc(le_mem_PoolRef_t pool, size_t size);
    /// @endcond

#   define le_mem_ForceVarAlloc(pool, size)                             \
        _le_mem_VarAllocTracer(pool,                                    \
                               size,                                    \
                               _le_mem_ForceVarAlloc,                   \
                               "le_mem_ForceVarAlloc",                  \
                               STRINGIZE(LE_FILENAME),                  \
                               __FUNCTION__,                            \
                               __LINE__)
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Attempts to allocate an object from a pool using the configured allocation failure behaviour
 * (force or assert).  Forced allocation will expand into the heap if the configured pool size is
 * exceeded, while assert allocation will abort the program with an error if the pool cannot satisfy
 * the request.
 *
 * @param  pool Pool from which the object is to be allocated.
 *
 * @return Pointer to the allocated object.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_MEM_ALLOC_FORCE
#   define le_mem_Alloc(pool)   le_mem_ForceAlloc(pool)
#elif LE_CONFIG_MEM_ALLOC_ASSERT
#   define le_mem_Alloc(pool)   le_mem_AssertAlloc(pool)
#else
#   error "No supported allocation scheme selected!"
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Attempts to allocate a variably-sized object from a pool using the configured allocation failure
 * behaviour (force or assert).  Forced allocation will expand into the heap if the configured pool
 * size is exceeded, while assert allocation will abort the program with an error if the pool cannot
 * satisfy the request.
 *
 * @param  pool Pool from which the object is to be allocated.
 * @param  size The size of block to allocate.
 *
 * @return Pointer to the allocated object.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_MEM_ALLOC_FORCE
#   define le_mem_VarAlloc(pool, size)  le_mem_ForceVarAlloc((pool), (size))
#elif LE_CONFIG_MEM_ALLOC_ASSERT
#   define le_mem_VarAlloc(pool, size)  le_mem_AssertVarAlloc((pool), (size))
#else
#   error "No supported allocation scheme selected!"
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Sets the number of objects that are added when le_mem_ForceAlloc expands the pool.
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
    le_mem_PoolRef_t    pool,       ///< [IN] Pool to set the number of objects for.
    size_t              numObjects  ///< [IN] Number of objects that is added when
                                    ///       le_mem_ForceAlloc expands the pool.
);


#if !LE_CONFIG_MEM_TRACE
    //----------------------------------------------------------------------------------------------
    /**
     * Releases an object.  If the object's reference count has reached zero, it will be destructed
     * and its memory will be put back into the pool for later reuse.
     *
     * @return
     *      Nothing.
     *
     * @warning
     *      - <b>Don't EVER access an object after releasing it.</b>  It might not exist anymore.
     *      - If the object has a destructor accessing a data structure shared by multiple
     *        threads, ensure you hold the mutex (or take other measures to prevent races) before
     *        releasing the object.
     */
    //----------------------------------------------------------------------------------------------
    void le_mem_Release
    (
        void*   objPtr  ///< [IN] Pointer to the object to be released.
    );
#else
    /// @cond HIDDEN_IN_USER_DOCS
    void _le_mem_Release(void* objPtr);
    /// @endcond

#   define le_mem_Release(objPtr)                                                                   \
            _le_mem_Trace(_le_mem_GetBlockPool(objPtr),                                             \
                          STRINGIZE(LE_FILENAME),                                                   \
                          __FUNCTION__,                                                             \
                          __LINE__,                                                                 \
                          "le_mem_Release",                                                         \
                          (objPtr));                                                                \
            _le_mem_Release(objPtr);
#endif


#if !LE_CONFIG_MEM_TRACE
    //----------------------------------------------------------------------------------------------
    /**
     * Increments the reference count on an object by 1.
     *
     * See @ref mem_ref_counting for more information.
     *
     * @return
     *      Nothing.
     */
    //----------------------------------------------------------------------------------------------
    void le_mem_AddRef
    (
        void*   objPtr  ///< [IN] Pointer to the object.
    );
#else
    /// @cond HIDDEN_IN_USER_DOCS
    void _le_mem_AddRef(void* objPtr);
    /// @endcond

#   define le_mem_AddRef(objPtr)                                                                    \
        _le_mem_Trace(_le_mem_GetBlockPool(objPtr),                                                 \
                      STRINGIZE(LE_FILENAME),                                                       \
                      __FUNCTION__,                                                                 \
                      __LINE__,                                                                     \
                      "le_mem_AddRef",                                                              \
                      (objPtr));                                                                    \
        _le_mem_AddRef(objPtr);
#endif

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
);

//----------------------------------------------------------------------------------------------
/**
 * Fetches the reference count on an object.
 *
 * See @ref mem_ref_counting for more information.
 *
 * @warning If using this in a multi-threaded application that shares memory pool objects
 *          between threads, steps must be taken to coordinate the threads (e.g., using a mutex)
 *          to ensure that the reference count value fetched remains correct when it is used.
 *
 * @return
 *      The reference count on the object.
 */
//----------------------------------------------------------------------------------------------
size_t le_mem_GetRefCount
(
    void*   objPtr  ///< [IN] Pointer to the object.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the destructor function for a specified pool.
 *
 * See @ref mem_destructors for more information.
 *
 * @return
 *      Nothing.
 */
//--------------------------------------------------------------------------------------------------
void le_mem_SetDestructor
(
    le_mem_PoolRef_t    pool,       ///< [IN] The pool.
    le_mem_Destructor_t destructor  ///< [IN] Destructor function.
);


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the statistics for a specified pool.
 *
 * @return
 *      Nothing.  Uses output parameter instead.
 */
//--------------------------------------------------------------------------------------------------
void le_mem_GetStats
(
    le_mem_PoolRef_t    pool,       ///< [IN] Pool where stats are to be fetched.
    le_mem_PoolStats_t* statsPtr    ///< [OUT] Pointer to where the stats will be stored.
);


//--------------------------------------------------------------------------------------------------
/**
 * Resets the statistics for a specified pool.
 *
 * @return
 *      Nothing.
 */
//--------------------------------------------------------------------------------------------------
void le_mem_ResetStats
(
    le_mem_PoolRef_t    pool        ///< [IN] Pool where stats are to be reset.
);


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
    char*               namePtr,    ///< [OUT] Buffer to store the name of the memory pool.
    size_t              bufSize     ///< [IN] Size of the buffer namePtr points to.
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the number of objects a specified pool can hold (this includes both the number of
 * free and in-use objects).
 *
 * @return
 *      Total number of objects.
 */
//--------------------------------------------------------------------------------------------------
size_t le_mem_GetObjectCount
(
    le_mem_PoolRef_t    pool        ///< [IN] Pool where number of objects is to be fetched.
);


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the size of the objects in a specified pool (in bytes).
 *
 * @return
 *      Object size, in bytes.
 */
//--------------------------------------------------------------------------------------------------
size_t le_mem_GetObjectSize
(
    le_mem_PoolRef_t    pool        ///< [IN] Pool where object size is to be fetched.
);


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
);


#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/** @cond HIDDEN_IN_USER_DOCS
 *
 * Internal function used to implement le_mem_FindPool() with automatic component scoping
 * of pool names.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t _le_mem_FindPool
(
    const char* componentName,  ///< [IN] Name of the component.
    const char* name            ///< [IN] Name of the pool inside the component.
);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Finds a pool based on the pool's name.
 *
 * @return
 *      Reference to the pool, or NULL if the pool doesn't exist.
 */
//--------------------------------------------------------------------------------------------------
static inline le_mem_PoolRef_t le_mem_FindPool
(
    const char* name            ///< [IN] Name of the pool inside the component.
)
{
    return _le_mem_FindPool(STRINGIZE(LE_COMPONENT_NAME), name);
}
#else
//--------------------------------------------------------------------------------------------------
/**
 * Finds a pool based on the pool's name.
 *
 * @return
 *      Reference to the pool, or NULL if the pool doesn't exist.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE le_mem_PoolRef_t le_mem_FindPool
(
    const char* name            ///< [IN] Name of the pool inside the component.
)
{
    return NULL;
}
#endif /* end LE_CONFIG_MEM_POOL_NAMES_ENABLED */


#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/** @cond HIDDEN_IN_USER_DOCS
 *
 * Internal function used to implement le_mem_CreateSubPool() with automatic component scoping
 * of pool names.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t _le_mem_CreateSubPool
(
    le_mem_PoolRef_t    superPool,  ///< [IN] Super-pool.
    const char*         componentName,  ///< [IN] Name of the component.
    const char*         name,       ///< [IN] Name of the sub-pool (will be copied into the
                                    ///   sub-pool).
    size_t              numObjects  ///< [IN] Number of objects to take from the super-pool.
);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Creates a sub-pool.
 *
 * See @ref mem_sub_pools for more information.
 *
 * @return
 *      Reference to the sub-pool.
 */
//--------------------------------------------------------------------------------------------------
static inline le_mem_PoolRef_t le_mem_CreateSubPool
(
    le_mem_PoolRef_t    superPool,      ///< [IN] Super-pool.
    const char*         name,           ///< [IN] Name of the sub-pool (will be copied into the
                                        ///   sub-pool).
    size_t              numObjects      ///< [IN] Number of objects to take from the super-pool.
)
{
    return _le_mem_CreateSubPool(superPool, STRINGIZE(LE_COMPONENT_NAME), name, numObjects);
}
#else /* if not LE_CONFIG_MEM_POOL_NAMES_ENABLED */
//--------------------------------------------------------------------------------------------------
/** @cond HIDDEN_IN_USER_DOCS
 *
 * Internal function used to implement le_mem_CreateSubPool() with automatic component scoping
 * of pool names.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t _le_mem_CreateSubPool
(
    le_mem_PoolRef_t    superPool,      ///< [IN] Super-pool.
    size_t              numObjects      ///< [IN] Number of objects to take from the super-pool.
);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Creates a sub-pool.
 *
 * See @ref mem_sub_pools for more information.
 *
 * @return
 *      Reference to the sub-pool.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE le_mem_PoolRef_t le_mem_CreateSubPool
(
    le_mem_PoolRef_t    superPool,      ///< [IN] Super-pool.
    const char*         name,           ///< [IN] Name of the sub-pool (will be copied into the
                                        ///   sub-pool).
    size_t              numObjects      ///< [IN] Number of objects to take from the super-pool.
)
{
    return _le_mem_CreateSubPool(superPool, numObjects);
}
#endif /* end LE_CONFIG_MEM_POOL_NAMES_ENABLED */


#if LE_CONFIG_MEM_POOL_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/** @cond HIDDEN_IN_USER_DOCS
 *
 * Internal function used to implement le_mem_CreateSubPool() with automatic component scoping
 * of pool names.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t _le_mem_CreateReducedPool
(
    le_mem_PoolRef_t    superPool,      ///< [IN] Super-pool.
    const char*         componentName,  ///< [IN] Name of the component.
    const char*         name,           ///< [IN] Name of the sub-pool (will be copied into the
                                        ///   sub-pool).
    size_t              numObjects,     ///< [IN] Number of objects to take from the super-pool.
    size_t              objSize         ///< [IN] Minimum size of objects in the subpool.
);
/// @endcond
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
static inline le_mem_PoolRef_t le_mem_CreateReducedPool
(
    le_mem_PoolRef_t    superPool,      ///< [IN] Super-pool.
    const char*         name,           ///< [IN] Name of the sub-pool (will be copied into the
                                        ///   sub-pool).
    size_t              numObjects,     ///< [IN] Minimum number of objects in the subpool
                                        ///< by default.
    size_t              objSize         ///< [IN] Minimum size of objects in the subpool.
)
{
    return _le_mem_CreateReducedPool(superPool, STRINGIZE(LE_COMPONENT_NAME), name, numObjects, objSize);
}
#else /* if not LE_CONFIG_MEM_POOL_NAMES_ENABLED */
//--------------------------------------------------------------------------------------------------
/** @cond HIDDEN_IN_USER_DOCS
 *
 * Internal function used to implement le_mem_CreateSubPool() with automatic component scoping
 * of pool names.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t _le_mem_CreateReducedPool
(
    le_mem_PoolRef_t    superPool,      ///< [IN] Super-pool.
    size_t              numObjects,     ///< [IN] Minimum number of objects in the subpool
                                        ///< by default.
    size_t              objSize         ///< [IN] Minimum size of objects in the subpool.
);
/// @endcond
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
LE_DECLARE_INLINE le_mem_PoolRef_t le_mem_CreateReducedPool
(
    le_mem_PoolRef_t    superPool,      ///< [IN] Super-pool.
    const char*         name,           ///< [IN] Name of the sub-pool (will be copied into the
                                        ///   sub-pool).
    size_t              numObjects,     ///< [IN] Minimum number of objects in the subpool
                                        ///< by default.
    size_t              objSize         ///< [IN] Minimum size of objects in the subpool.
)
{
    return _le_mem_CreateReducedPool(superPool, numObjects, objSize);
}
#endif /* end LE_CONFIG_MEM_POOL_NAMES_ENABLED */


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a sub-pool.
 *
 * See @ref mem_sub_pools for more information.
 *
 * @return
 *      Nothing.
 */
//--------------------------------------------------------------------------------------------------
void le_mem_DeleteSubPool
(
    le_mem_PoolRef_t    subPool     ///< [IN] Sub-pool to be deleted.
);

#if LE_CONFIG_RTOS
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
);

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
);

#endif /* end LE_CONFIG_RTOS */


#endif // LEGATO_MEM_INCLUDE_GUARD
