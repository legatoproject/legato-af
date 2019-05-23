/**
 * @page c_threading Thread Control API
 *
 * @subpage le_thread.h "API Reference"
 *
 * <HR>
 *
 * Generally, using single-threaded, event-driven programming (registering callbacks to be called
 * by an event handling loop running in a single thread) is more efficient than using
 * multiple threads. With single-threaded, event driven designs:
 *  - there's no CPU time spent switching between threads.
 *  - there's only one copy of thread-specific memory objects, like the procedure call stack.
 *  - there's no need to use thread synchronization mechanisms, like mutexes, to prevent race
 *      conditions between threads.
 *
 * Sometimes, this style doesn't fit well with a problem being solved,
 * so you're forced to implement workarounds that severely complicate
 * the software design.  In these cases, it is far better to take advantage of multi-threading
 *  to simplify the design, even if it means that the program uses more memory or more
 * CPU cycles.  In some cases, the workarounds required to avoid multi-threading will
 * cost more memory and/or CPU cycles than using multi-threading would.
 *
 * But you must <b> be careful </b> with multi-threading. Some of the most tenacious,
 * intermittent defects known to humankind have resulted from the misuse of multi-threading.
 * Ensure you know what you are doing.
 *
 * @section threadCreating Creating a Thread
 *
 * To create a thread,  call @c le_thread_Create().
 *
 * All threads are @b named for two reasons:
 *   -# To make it possible to address them by name.
 *   -# For diagnostics.
 *
 * Threads are created in a suspended state.  In this state, attributes like
 * scheduling priority and stack size can use the appropriate "Set" functions.
 * All attributes have default values so it is not necessary to set any
 * attributes (other than the name and main function address, which are passed into
 * le_thread_Create() ).  When all attributes have been set, the thread can be started by calling
 * le_thread_Start().
 *
 * @warning It is assumed that if a thread @e T1 creates another thread @e T2 then @b only thread
 *          @e T1 will set the attributes and start thread @e T2.  No other thread should try
 *          to set any attributes of @e T2 or try to start it.
 *
 *
 * @section threadTerminating Terminating a Thread
 *
 * Threads can terminate themselves by:
 *  - returning from their main function
 *  - calling le_thread_Exit().
 *
 * Threads can also tell other threads to terminate by "canceling" them;  done
 * through a call to @c le_thread_Cancel().
 *
 * If a thread terminates itself, and it is "joinable", it can pass a @c void* value to another
 * thread that "joins" with it.  See @ref threadJoining for more information.
 *
 * Canceling a thread may not cause the thread to terminate immediately.  If it is
 * in the middle of doing something that can't be interrupted, it will not terminate until it
 * is finished.  See 'man 7 pthreads' for more information on cancellation
 * and cancellation points.
 *
 * To prevent cancellation during a critical section (e.g., when a mutex lock is held),
 * pthread_setcancelstate() can be called.  If a cancellation request is made (by calling
 * le_thread_Cancel() or <c>pthread_cancel()</c>), it will be blocked and remain in a pending state
 * until cancellation is unblocked (also using pthread_setcancelstate()), at which time the thread
 * will be immediately cancelled.
 *
 *
 * @section threadJoining Joining
 *
 * Sometimes, you want single execution thread split (fork) into separate
 * threads of parallel execution and later join back together into one thread later.  Forking
 * is done by creating and starting a thread. Joining is done by a call to le_thread_Join().
 * le_thread_Join(T) blocks the calling thread until thread T exits.
 *
 * For a thread to be joinable, it must have its "joinable" attribute set (using
 * le_thread_SetJoinable()) prior to being started.  Normally, when a thread terminates, it
 * disappears.  But, a joinable thread doesn't disappear until another thread "joins" with it.
 * This also means that if a thread is joinable, someone must join with it, or its
 * resources will never get cleaned up (until the process terminates).
 *
 * le_thread_Join() fetches the return/exit value of the thread that it joined with.
 *
 *
 * @section threadLocalData Thread-Local Data
 *
 * Often, you want data specific to a particular thread.  A classic example
 * of is the ANSI C variable @c errno. If one instance of @c errno was shared by all the
 * threads in the process, then it would essentially become useless in a multi-threaded program
 * because it would be impossible to ensure another thread hadn't killed @c errno before
 * its value could be read.  As a result, POSIX has mandated that @c errno be a @a thread-local
 * variable; each thread has its own unique copy of @c errno.
 *
 * If a component needs to make use of other thread-local data, it can do so using the pthread
 * functions pthread_key_create(), pthread_getspecific(), pthread_setspecific(),
 * pthread_key_delete().  See the pthread man pages for more details.
 *
 *
 * @section threadSynchronization Thread Synchronization
 *
 * Nasty multi-threading defects arise as a result of thread synchronization, or a
 * lack of synchronization.  If threads share data, they @b MUST be synchronized with each other to
 * avoid destroying that data and incorrect thread behaviour.
 *
 * @warning This documentation assumes that the reader is familiar with multi-thread synchronization
 * techniques and mechanisms.
 *
 * The Legato C APIs provide the following thread synchronization mechanisms:
 *  - @ref c_mutex
 *  - @ref c_semaphore
 *  - @ref c_messaging
 *
 *
 * @section threadDestructors Thread Destructors
 *
 * When a thread dies, some clean-up action is needed (e.g., a connection
 * needs to be closed or some objects need to be released).  If a thread doesn't always terminate
 * the same way (e.g., if it might be canceled by another thread or exit in several places due
 * to error detection code), then a clean-up function (destructor) is probably needed.
 *
 * A Legato thread can use @c le_thread_AddDestructor() to register a function to be called
 * by that thread just before it terminates.
 *
 * A parent thread can also call @c le_thread_AddChildDestructor() to register
 * a destructor for a child thread before it starts the child thread.
 *
 * Multiple destructors can be registered for the same thread.  They will be called in reverse
 * order of registration (i.e, the last destructor to be registered will be called first).
 *
 * A Legato thread can also use le_thread_RemoveDestructor() to remove its own destructor
 * function that it no longer wants called in the event of its death.  (There is no way to remove
 * destructors from other threads.)
 *
 *
 * @section threadLegatoizing Using Legato APIs from Non-Legato Threads
 *
 * If a thread is started using some other means besides le_thread_Start() (e.g., if
 * pthread_create() is used directly), then the Legato thread-specific data will not have
 * been initialized for that thread.  Therefore, if that thread tries to call some Legato APIs,
 * a fatal error message like, "Legato threading API used in non-Legato thread!" may be seen.
 *
 * To work around this, a "non-Legato thread" can call le_thread_InitLegatoThreadData() to
 * initialize the thread-specific data that the Legato framework needs.
 *
 * If you have done this for a thread, and that thread will die before the process it is inside
 * dies, then that thread must call le_thread_CleanupLegatoThreadData() before it exits.  Otherwise
 * the process will leak memory.  Furthermore, if the thread will ever be cancelled by another
 * thread before the process dies, a cancellation clean-up handler can be used to ensure that
 * the clean-up is done, if the thread's cancellation type is set to "deferred".
 * See 'man 7 pthreads' for more information on cancellation and cancellation points.
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


/** @file le_thread.h
 *
 * Legato @ref c_threading include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_THREAD_INCLUDE_GUARD
#define LEGATO_THREAD_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Reference to a thread of execution.
 *
 * @note NULL can be used as an invalid value.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_thread* le_thread_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Thread priority levels.
 *
 * Real-time priority levels should be avoided unless absolutely necessary for the application.
 * They are privileged levels and will therefore not be allowed unless the application is executed
 * by an identity with the appropriate permissions.  If a thread running at a real-time priority
 * level does not block, no other thread at a lower priority level will run, so be careful with
 * these.
 *
 * @note Higher numbers are higher priority.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_THREAD_PRIORITY_IDLE = 0,    ///< Lowest priority level. Only runs when nothing else to do.

    LE_THREAD_PRIORITY_LOW,         ///< Low, non-realtime priority level.
                                    ///< Low, medium, high: intended for normal processes that
                                    ///< contend for the CPU. Processes with these priorities don't
                                    ///< preempt each other, but their priorities affect how they're
                                    ///< inserted into the scheduling queue (high to low).
    LE_THREAD_PRIORITY_MEDIUM,      ///< Medium, non-real-time priority level. THIS IS THE DEFAULT.
    LE_THREAD_PRIORITY_HIGH,        ///< High, non-real-time priority level.

    LE_THREAD_PRIORITY_RT_1,        ///< Real-time priority level 1.  The lowest realtime priority
                                    ///  level.
    LE_THREAD_PRIORITY_RT_2,        ///< Real-time priority level 2.
    LE_THREAD_PRIORITY_RT_3,        ///< Real-time priority level 3.
    LE_THREAD_PRIORITY_RT_4,        ///< Real-time priority level 4.
    LE_THREAD_PRIORITY_RT_5,        ///< Real-time priority level 5.
    LE_THREAD_PRIORITY_RT_6,        ///< Real-time priority level 6.
    LE_THREAD_PRIORITY_RT_7,        ///< Real-time priority level 7.
    LE_THREAD_PRIORITY_RT_8,        ///< Real-time priority level 8.
    LE_THREAD_PRIORITY_RT_9,        ///< Real-time priority level 9.
    LE_THREAD_PRIORITY_RT_10,       ///< Real-time priority level 10.
    LE_THREAD_PRIORITY_RT_11,       ///< Real-time priority level 11.
    LE_THREAD_PRIORITY_RT_12,       ///< Real-time priority level 12.
    LE_THREAD_PRIORITY_RT_13,       ///< Real-time priority level 13.
    LE_THREAD_PRIORITY_RT_14,       ///< Real-time priority level 14.
    LE_THREAD_PRIORITY_RT_15,       ///< Real-time priority level 15.
    LE_THREAD_PRIORITY_RT_16,       ///< Real-time priority level 16.
    LE_THREAD_PRIORITY_RT_17,       ///< Real-time priority level 17.
    LE_THREAD_PRIORITY_RT_18,       ///< Real-time priority level 18.
    LE_THREAD_PRIORITY_RT_19,       ///< Real-time priority level 19.
    LE_THREAD_PRIORITY_RT_20,       ///< Real-time priority level 20.
    LE_THREAD_PRIORITY_RT_21,       ///< Real-time priority level 21.
    LE_THREAD_PRIORITY_RT_22,       ///< Real-time priority level 22.
    LE_THREAD_PRIORITY_RT_23,       ///< Real-time priority level 23.
    LE_THREAD_PRIORITY_RT_24,       ///< Real-time priority level 24.
    LE_THREAD_PRIORITY_RT_25,       ///< Real-time priority level 25.
    LE_THREAD_PRIORITY_RT_26,       ///< Real-time priority level 26.
    LE_THREAD_PRIORITY_RT_27,       ///< Real-time priority level 27.
    LE_THREAD_PRIORITY_RT_28,       ///< Real-time priority level 28.
    LE_THREAD_PRIORITY_RT_29,       ///< Real-time priority level 29.
    LE_THREAD_PRIORITY_RT_30,       ///< Real-time priority level 30.
    LE_THREAD_PRIORITY_RT_31,       ///< Real-time priority level 31.
    LE_THREAD_PRIORITY_RT_32        ///< Real-time priority level 32.
}
le_thread_Priority_t;

#define LE_THREAD_PRIORITY_RT_LOWEST    LE_THREAD_PRIORITY_RT_1     ///< Lowest real-time priority.
#define LE_THREAD_PRIORITY_RT_HIGHEST   LE_THREAD_PRIORITY_RT_32    ///< Highest real-time priority.

//--------------------------------------------------------------------------------------------------
/**
 * @deprecated
 *
 * LE_THREAD_PRIORITY_NORMAL is deprecated, use LE_THREAD_PRIORITY_MEDIUM instead.
 */
//--------------------------------------------------------------------------------------------------
#define LE_THREAD_PRIORITY_NORMAL       LE_THREAD_PRIORITY_MEDIUM


//--------------------------------------------------------------------------------------------------
/**
 * Main functions for threads must look like this:
 *
 * @param   context [IN] Context value that was passed to le_thread_Create().
 *
 * @return  Thread result value. If the thread is joinable, then this value can be obtained by
 *          another thread through a call to vt_thread_Join().  Otherwise, the return value is ignored.
 */
//--------------------------------------------------------------------------------------------------
typedef void* (* le_thread_MainFunc_t)
(
    void* context   ///< See parameter documentation above.
);


#if LE_CONFIG_THREAD_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/**
 * Creates a new Legato thread of execution.  After creating the thread, you have the opportunity
 * to set attributes before it starts.  It won't start until le_thread_Start() is called.
 *
 *  @param[in]  name        Thread name (will be copied, so can be temporary).
 *  @param[in]  mainFunc    Thread's main function.
 *  @param[in]  context     Value to pass to mainFunc when it is called.
 *
 *  @return A reference to the thread (doesn't return if fails).
 */
//--------------------------------------------------------------------------------------------------
le_thread_Ref_t le_thread_Create
(
    const char*             name,
    le_thread_MainFunc_t    mainFunc,
    void*                   context
);
#else /* if not LE_CONFIG_THREAD_NAMES_ENABLED */
/// @cond HIDDEN_IN_USER_DOCS
//--------------------------------------------------------------------------------------------------
/**
 * Internal function used to implement le_thread_Create().
 */
//--------------------------------------------------------------------------------------------------
le_thread_Ref_t _le_thread_Create(le_thread_MainFunc_t mainFunc, void* context);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Creates a new Legato thread of execution.  After creating the thread, you have the opportunity
 * to set attributes before it starts.  It won't start until le_thread_Start() is called.
 *
 *  @param[in]  name        Thread name (will be copied, so can be temporary).
 *  @param[in]  mainFunc    Thread's main function.
 *  @param[in]  context     Value to pass to mainFunc when it is called.
 *
 *  @return A reference to the thread (doesn't return if fails).
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE le_thread_Ref_t le_thread_Create
(
    const char*             name,
    le_thread_MainFunc_t    mainFunc,
    void*                   context
)
{
    return _le_thread_Create(mainFunc, context);
}
#endif /* end LE_CONFIG_THREAD_NAMES_ENABLED */


//--------------------------------------------------------------------------------------------------
/**
 * Sets the priority of a thread.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_OUT_OF_RANGE if the priority level requested is out of range.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_thread_SetPriority
(
    le_thread_Ref_t         thread,     ///< [IN]
    le_thread_Priority_t    priority    ///< [IN]
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the stack size of a thread.
 *
 * @note It's generally not necessary to set the stack size.  Some reasons why you might are:
 *         - to increase it beyond the system's default stack size to prevent overflow
 *              for a thread that makes extremely heavy use of the stack;
 *         - to decrease it to save memory when:
 *            - running in a system that does not support virtual memory
 *            - the thread has very tight real-time constraints that require that the stack
 *                  memory be locked into physical memory to avoid page faults.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_OVERFLOW if the stack size requested is too small.
 *      - LE_OUT_OF_RANGE if the stack size requested is too large.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_thread_SetStackSize
(
    le_thread_Ref_t     thread,     ///< [IN]
    size_t              size        ///< [IN] Stack size, in bytes.  May be rounded up to the
                                    ///       nearest virtual memory page size.
);


//--------------------------------------------------------------------------------------------------
/**
 *  Define a static thread stack region.
 *
 *  @param  name    Stack variable name.
 *  @param  bytes   Number of bytes in the stack.
 */
//--------------------------------------------------------------------------------------------------
#define LE_THREAD_DEFINE_STATIC_STACK(name, bytes)                                  \
    static uint8_t _thread_stack_##name[LE_THREAD_STACK_EXTRA_SIZE +                \
        ((bytes) < LE_THREAD_STACK_MIN_SIZE ? LE_THREAD_STACK_MIN_SIZE : (bytes))]  \
        __attribute__((aligned(LE_THREAD_STACK_ALIGNMENT)))


//--------------------------------------------------------------------------------------------------
/**
 *  Set a static stack for a thread.
 *
 *  @see le_thread_SetStack() for details.
 *
 *  @param  thread  Thread to set the stack for.
 *  @param  name    Stack variable name that was previously passed to
 *                  LE_THREAD_DEFINE_STATIC_STACK().
 *
 *  @return Return value of le_thread_SetStack().
 */
//--------------------------------------------------------------------------------------------------
#define LE_THREAD_SET_STATIC_STACK(thread, name) \
    le_thread_SetStack((thread), &_thread_stack_##name, sizeof(_thread_stack_##name))


//--------------------------------------------------------------------------------------------------
/**
 *  Sets the stack of a thread.
 *
 *  Setting the stack explicitly allows the caller to control the memory allocation of the thread's
 *  stack and, in some cases, control data.  This can be useful for allocating the space out of
 *  static memory, for example.
 *
 *  The macro LE_THREAD_DEFINE_STATIC_STACK() may be used to create a statically allocated stack for
 *  use with this function, and LE_THREAD_SET_STATIC_STACK() may be used to call it properly.
 *
 *  @attention  In general, this function is only useful on embedded, RTOS based systems in order to
 *              perform up-front allocation of thread resources.  On more capable systems it is
 *              safer to allow the operating system to set up the stack (which may optionally be
 *              sized using le_thread_SetStackSize()).
 *
 *  @return
 *      - LE_OK if successful.
 *      - LE_BAD_PARAMETER if the size or stack is invalid (NULL or improperly aligned).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_thread_SetStack
(
    le_thread_Ref_t  thread,    ///< [IN] Thread instance.
    void            *stack,     ///< [IN] Address of the lowest byte of the stack.  This must be
                                ///       appropriately aligned (LE_THREAD_DEFINE_STATIC_STACK()
                                ///       will do this).
    size_t           size       ///< [IN] Stack size, in bytes.
);


//--------------------------------------------------------------------------------------------------
/**
 * Makes a thread "joinable", meaning that when it finishes, it will remain in existence until
 * another thread "joins" with it by calling le_thread_Join().  By default, threads are not
 * joinable and will be destroyed automatically when they finish.
 */
//--------------------------------------------------------------------------------------------------
void le_thread_SetJoinable
(
    le_thread_Ref_t         thread  ///< [IN]
);


//--------------------------------------------------------------------------------------------------
/**
 * Starts a new Legato execution thread.  After creating the thread, you have the opportunity
 * to set attributes before it starts.  It won't start until le_thread_Start() is called.
 */
//--------------------------------------------------------------------------------------------------
void le_thread_Start
(
    le_thread_Ref_t     thread      ///< [IN]
);


//--------------------------------------------------------------------------------------------------
/**
 * "Joins" the calling thread with another thread.  Blocks the calling thread until the other
 * thread finishes.
 *
 * After a thread has been joined with, its thread reference is no longer valid and must never
 * be used again.
 *
 * The other thread's result value (the value it returned from its main function or passed into
 * le_thread_Exit()) can be obtained.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_DEADLOCK if a thread tries to join with itself or two threads try to join each other.
 *      - LE_NOT_FOUND if the other thread doesn't exist.
 *      - LE_NOT_POSSIBLE if the other thread can't be joined with.
 * @deprecated the result code LE_NOT_POSSIBLE is scheduled to be removed before 15.04
 *
 * @warning The other thread must be "joinable".  See le_thread_SetJoinable();
 *
 * @warning It's an error for two or more threads try to join with the same thread.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_thread_Join
(
    le_thread_Ref_t     thread,         ///< [IN]
    void**              resultValuePtr  ///< [OUT] Ptr to where the finished thread's result value
                                        ///        will be stored.  Can be NULL if the result is
                                        ///        not needed.
);


//--------------------------------------------------------------------------------------------------
/**
 * Terminates the calling thread.
 */
//--------------------------------------------------------------------------------------------------
void le_thread_Exit
(
    void*   resultValue     ///< [IN] Result value.  If this thread is joinable, this result
                            ///         can be obtained by another thread calling le_thread_Join()
                            ///         on this thread.
);


//--------------------------------------------------------------------------------------------------
/**
 * Tells another thread to terminate.  Returns immediately, but the termination of the
 * thread happens asynchronously and is not guaranteed to occur when this function returns.
 *
 * @note This function is not available on RTOS.

 *
 * @return
 *      - LE_OK if successful.
 *      - LE_NOT_FOUND if the thread doesn't exist.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_result_t le_thread_Cancel
(
    le_thread_Ref_t threadToCancel  ///< [IN] Thread to cancel.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the calling thread's thread reference.
 *
 * @return  Calling thread's thread reference.
 */
//--------------------------------------------------------------------------------------------------
le_thread_Ref_t le_thread_GetCurrent
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the name of a given thread.
 */
//--------------------------------------------------------------------------------------------------
void le_thread_GetName
(
    le_thread_Ref_t threadRef,  ///< [IN] Thread to get the name for.
    char* buffPtr,              ///< [OUT] Buffer to store the name of the thread.
    size_t buffSize             ///< [IN] Size of the buffer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the name of the calling thread.
 */
//--------------------------------------------------------------------------------------------------
const char* le_thread_GetMyName
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Destructor functions for threads must look like this:
 *
 * @param context [IN] Context parameter that was passed into le_thread_SetDestructor() when
 *                      this destructor was registered.
 */
//--------------------------------------------------------------------------------------------------
typedef void (* le_thread_Destructor_t)
(
    void* context   ///< [IN] Context parameter that was passed into le_thread_SetDestructor() when
                    ///       this destructor was registered.
);


//--------------------------------------------------------------------------------------------------
/**
 * Reference to a registered destructor function.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_thread_Destructor* le_thread_DestructorRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Registers a destructor function for the calling thread.  The destructor will be called by that
 * thread just before it terminates.
 *
 * A thread can register (or remove) its own destructor functions any time.
 *
 * @return Reference to the destructor that can be passed to le_thread_RemoveDestructor().
 *
 * See @ref threadDestructors for more information on destructors.
 */
//--------------------------------------------------------------------------------------------------
le_thread_DestructorRef_t le_thread_AddDestructor
(
    le_thread_Destructor_t  destructor, ///< [IN] Function to be called.
    void*                   context     ///< [IN] Parameter to pass to the destructor.
);


//--------------------------------------------------------------------------------------------------
/**
 * Registers a destructor function for a child thread.  The destructor will be called by the
 * child thread just before it terminates.
 *
 * This can only be done before the child thread is started.  After that, only the child thread
 * can add its own destructors.
 *
 * The reason for allowing another thread to register a destructor function is to
 * avoid a race condition that can cause resource leakage when a parent thread passes dynamically
 * allocated resources to threads that they create. This is only a problem if the child thread
 * is expected to release the resources when they are finished with them, and the child thread
 * may get cancelled at any time.
 *
 * For example, a thread @e T1 could allocate an object from a memory pool, create a thread @e T2,
 * and pass that object to @e T2 for processing and release.  @e T2 could register a destructor
 * function to release the resource whenever it terminates, whether through cancellation or
 * normal exit.  But, if it's possible that @e T2 could get cancelled before it even has a
 * chance to register a destructor function for itself, the memory pool object could never get
 * released.  So, we allow @e T1 to register a destructor function for @e T2 before starting @e T2.
 *
 * See @ref threadDestructors for more information on destructors.
 */
//--------------------------------------------------------------------------------------------------
void le_thread_AddChildDestructor
(
    le_thread_Ref_t         thread,     ///< [IN] Thread to attach the destructor to.
    le_thread_Destructor_t  destructor, ///< [IN] Function to be called.
    void*                   context     ///< [IN] Parameter to pass to the destructor.
);


//--------------------------------------------------------------------------------------------------
/**
 * Removes a destructor function from the calling thread's list of destructors.
 */
//--------------------------------------------------------------------------------------------------
void le_thread_RemoveDestructor
(
    le_thread_DestructorRef_t  destructor ///< [in] Reference to the destructor to remove.
);


#if LE_CONFIG_THREAD_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/**
 * Initialize the thread-specific data needed by the Legato framework for the calling thread.
 *
 * This is used to turn a non-Legato thread (a thread that was created using a non-Legato API,
 * such as pthread_create() ) into a Legato thread.
 *
 *  @param[in]  name    A name for the thread (will be copied, so can be temporary).
 *
 *  @note This is not needed if the thread was started using le_thread_Start().
 **/
//--------------------------------------------------------------------------------------------------
void le_thread_InitLegatoThreadData
(
    const char* name
);
#else /* if not LE_CONFIG_THREAD_NAMES_ENABLED */
/// @cond HIDDEN_IN_USER_DOCS
//--------------------------------------------------------------------------------------------------
/**
 * Internal function used to implement le_thread_InitLegatoThreadData().
 */
//--------------------------------------------------------------------------------------------------
void _le_thread_InitLegatoThreadData(void);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Initialize the thread-specific data needed by the Legato framework for the calling thread.
 *
 * This is used to turn a non-Legato thread (a thread that was created using a non-Legato API,
 * such as pthread_create() ) into a Legato thread.
 *
 *  @param[in]  name    A name for the thread (will be copied, so can be temporary).
 *
 *  @note This is not needed if the thread was started using le_thread_Start().
 **/
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE void le_thread_InitLegatoThreadData
(
    const char* name
)
{
    _le_thread_InitLegatoThreadData();
}
#endif /* end LE_CONFIG_THREAD_NAMES_ENABLED */


//--------------------------------------------------------------------------------------------------
/**
 * Clean-up the thread-specific data that was initialized using le_thread_InitLegatoThreadData().
 *
 * To prevent memory leaks, this must be called by the thread when it dies (unless the whole
 * process is dying).
 *
 * @note This is not needed if the thread was started using le_thread_Start().
 **/
//--------------------------------------------------------------------------------------------------
void le_thread_CleanupLegatoThreadData
(
    void
);


#endif // LEGATO_THREAD_INCLUDE_GUARD
