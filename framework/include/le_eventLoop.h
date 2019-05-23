/**
 * @page c_eventLoop Event Loop API
 *
 * @subpage le_eventLoop.h "API Reference"
 *
 * <HR>
 *
 * The Event Loop API supports the event-driven programming model, which is favoured in Legato
 * (but not forced).  Each thread that uses this system has a central <b>event loop</b>
 * which calls <b>event handler</b> functions in response to <b>event reports</b>.
 *
 * Software components register their event handler functions with the event system (either
 * directly through the Event Loop API or indirectly through other APIs that use the Event Loop API)
 * so the central event loop knows the functions to call in response to defined events.
 *
 * Every event loop has an <b>event queue</b>, which is a queue of events waiting to be handled by
 * that event loop.
 *
 * @note When the process dies, all events, event loops, queues, reports, and handlers will be
 * automatically cleared.
 *
 * The following different usage patterns are supported by the Event Loop API:
 *
 * @ref c_event_deferredFunctionCalls <br>
 * @ref c_event_dispatchingToOtherThreads <br>
 * @ref c_event_publishSubscribe <br>
 * @ref c_event_layeredPublishSubscribe <br>
 *
 * Other Legato C Runtime Library APIs using the event loop include:
 *
 * @ref c_fdMonitor <br>
 * @ref c_timer <br>
 * @ref c_args <br>
 * @ref c_signals <br>
 * @ref c_messaging <br>
 *
 *
 * @section c_event_deferredFunctionCalls Deferred Function Calls
 *
 * A basic Event Queue usage is to queue a function for the Event Loop to call
 * later (when that function gets to the head of the Event Queue) by calling l@c e_event_QueueFunction().
 *
 * This code sample has a component initialization function queueing another function
 * to be call later, by the process's main thread when the Event Loop is running.  Two
 * parameters are needed by the deferred function.  The third is just filled with NULL and ignored
 * by the deferred function.
 *
 * @code
 * static void MyDeferredFunction
 * (
 *     void* param1Ptr,
 *     void* param2Ptr
 * )
 * {
 *     // Type cast the parameters to what they really are and do whatever it is that
 *     // I need to do with them.
 * }
 *
 * ...
 *
 * COMPONENT_INIT
 * {
 *     le_event_QueueFunction(MyDeferredFunction, firstParamPtr, secondParamPtr);
 * }
 * @endcode
 *
 * Deferred function calls are useful when implementing APIs with asynchronous
 * result call-backs. If an error is detected before the API function returns, it can't just
 * call the call-back directly, because it could cause re-entrancy problems in the client code
 * or cause recursive loops.  Instead of forcing the API function to return an error code
 * in special cases (which will increase the client's code complexity and may leak API
 * implementation details to the client), the API function can defers executing the
 * call-back until later by queuing an error handling function onto the Event Queue.
 *
 * @section c_event_dispatchingToOtherThreads Dispatching Function Execution to Other Threads
 *
 * In multi-threaded programs, sometimes the implementor needs
 * to ask another thread to run a function because:
 * - The function to be executed takes a long time, but doesn't have to be done at a high priority.
 * - A call needs to be made into a non-thread-safe API function.
 * - A blocking function needs to be called, but the current thread can't afford to block.
 *
 * To assist with this, the Event Loop API provides @c le_event_QueueFunctionToThread(). It
 * works the same as le_event_QueueFunction(), except that it queues the function onto a
 * specific thread's Event Queue.
 *
 * If the other thread isn't running the Event Loop, then the queued function will
 * never be executed.
 *
 * This code sample shows two arguments started by the process's main
 * thread, and executed in the background by a low-priority thread. The result is reported back
 * to the client through a completion callback running in the same thread that requested that the
 * computation be performed.
 *
 * @code
 * static le_mem_PoolRef_t ComputeRequestPool;
 * static le_thread_Ref_t LowPriorityThreadRef;
 *
 * typedef struct
 * {
 *     size_t           arg1;                                   // First argument
 *     size_t           arg2;                                   // Second argument
 *     ssize_t          result;                                 // The result
 *     void           (*completionCallback)(ssize_t result);    // The client's completion callback
 *     le_thread_Ref_t  requestingThreadRef;                    // The client's thread.
 * }
 * ComputeRequest_t;
 *
 * // Main function of low-priority background thread.
 * static void* LowPriorityThreadMain
 * (
 *     void* contextPtr // not used.
 * )
 * {
 *     le_event_RunLoop();
 * }
 *
 * COMPONENT_INIT
 * {
 *     ComputeRequestPool = le_mem_CreatePool("Compute Request", sizeof(ComputeRequest_t));
 *
 *     LowPriorityThreadRef = le_thread_Create("Background Computation Thread",
 *                                             LowPriorityThreadMain,
 *                                             NULL);
 *     le_thread_SetPriority(LowPriorityThreadRef, LE_THREAD_PRIORITY_IDLE);
 *     le_thread_Start(LowPriorityThreadRef);
 * }
 *
 * // This function gets run by a low-priority, background thread.
 * static void ComputeResult
 * (
 *     void* param1Ptr, // request object pointer
 *     void* param2Ptr  // not used
 * )
 * {
 *     ComputeRequest_t* requestPtr = param1Ptr;
 *
 *     requestPtr->result = DoSomeReallySlowComputation(requestPtr->arg1, requestPtr->arg2);
 *
 *     le_event_QueueFunctionToThread(requestPtr->requestingThreadRef,
 *                                    ProcessResult,
 *                                    requestPtr,
 *                                    NULL);
 * }
 *
 * // This function gets called by a component running in the main thread.
 * static void ComputeResultInBackground
 * (
 *      size_t arg1,
 *      size_t arg2,
 *      void (*completionCallback)(ssize_t result)
 * )
 * {
 *     ComputeRequest_t* requestPtr = le_mem_ForceAlloc(ComputeRequestPool);
 *     requestPtr->arg1 = arg1;
 *     requestPtr->arg2 = arg2;
 *     requestPtr->requestingThreadRef = le_thread_GetCurrent();
 *     requestPtr->completionCallback = completionCallback;
 *     le_event_QueueFunctionToThread(LowPriorityThreadRef,
 *                                    ComputeResult,
 *                                    requestPtr,
 *                                    NULL);
 * }
 *
 * // This function gets run by the main thread.
 * static void ProcessResult
 * (
 *     void* param1Ptr, // request object pointer
 *     void* param2Ptr  // not used
 * )
 * {
 *     ComputeRequest_t* requestPtr = param1Ptr;
 *     completionCallback(requestPtr->result);
 *     le_mem_Release(requestPtr);
 * }
 * @endcode
 *
 * @section c_event_publishSubscribe Publish-Subscribe Events
 *
 * In the publish-subscribe pattern, someone publishes information and if anyone cares about
 * that information, they subscribe to receive it. The publisher doesn't have to know whether
 * anything is listening, or how many subscribers might be listening.
 * Likewise, the subscribers don't have to know whether anything is publishing or how many
 * publishers there might be.  This decouples publishers and subscribers.
 *
 * Subscribers @b add handlers for events and wait for those handlers to be executed.
 *
 * Publishers @b report events.
 *
 * When an event report reaches the front of an Event Queue, the Event Loop will pop it from the
 * queue and call any handlers that have been registered for that event.
 *
 * Events are identified using an <b> Event ID </b> created by calling
 * @c le_event_CreateId() before registering an handler for that event or report.
 * Any thread within the process with an Event ID can register a handler or report
 * events.
 *
 * @note These Event IDs are only valid within the process where they were created. The
 *       Event Loop API can't be used for inter-process communication (IPC).
 *
 * @code
 * le_event_Id_t eventId = le_event_CreateId("MyEvent", sizeof(MyEventReport_t));
 * @endcode
 *
 * Event reports can carry a payload. The size and format of the payload depends on the type of
 * event.  For example, reports of temperature changes may need to carry the new temperature.
 * To support this, @c le_event_CreateId() takes the payload size as a parameter.
 *
 * To report an event, the publisher builds their report payload in their own buffer and passes
 * a pointer to that buffer (and its size) to @c le_event_Report():
 *
 * @code
 * MyEventReport_t report;
 * ...     // Fill in the event report.
 * le_event_Report(EventId, &report, sizeof(report));
 * @endcode
 *
 * This  results in the report getting queued to the Event Queues of all threads with
 * handlers registered for that event ID.
 *
 * To register a handler, the subscriber calls @c le_event_AddHandler().
 *
 * @note    It's okay to have a payload size of zero, in which case NULL can be passed into
 *          le_event_Report().
 *
 * @code
 * le_event_HandlerRef_t handlerRef = le_event_AddHandler("MyHandler", eventId, MyHandlerFunc);
 * @endcode
 *
 * When an event report reaches the front of a thread's Event Queue, that thread's Event Loop
 * reads the report and then:
 * - Calls the handler functions registered by that thread.
 * - Points to the report payload passed to the handler as a parameter.
 * - Reports the payload was deleted on return, so the handler
 * function must copy any contents to keep.
 *
 * @code
 * static void MyHandlerFunc
 * (
 *     void* reportPayloadPtr
 * )
 * {
 *     MyEventReport_t* reportPtr = reportPayloadPtr;
 *     // Process the report.
 *     ...
 * }
 * @endcode
 *
 * Another opaque pointer, called the <b> context pointer </b> can be set for
 * the handler using @c le_event_SetContextPtr().  When the handler function is called, it can call
 * le_event_GetContextPtr() to fetch the context pointer.
 *
 * @code
 * static void MyHandlerFunc
 * (
 *     void* reportPayloadPtr
 * )
 * {
 *     MyEventReport_t* reportPtr = reportPayloadPtr;
 *     MyContext_t* contextPtr = le_event_GetContextPtr();
 *
 *     // Process the report.
 *     ...
 * }
 *
 * COMPONENT_INIT
 * {
 *     MyEventId = le_event_CreateId("MyEvent", sizeof(MyEventReport_t));
 *
 *     MyHandlerRef = le_event_AddHandler("MyHandler", MyEventId, MyHandlerFunc);
 *     le_event_SetContextPtr(MyHandlerRef, sizeof(float));
 * }
 * @endcode
 *
 * Finally, le_event_RemoveHandler() can be used to remove an event handler registration,
 * if necessary.
 *
 * @code
 * le_event_RemoveHandler(MyHandlerRef);
 * @endcode
 *
 * If a handler is removed after the report for that event has been added to the event queue, but
 * before the report reaches the head of the queue, then the handler will not be called.
 *
 * @note To prevent race conditions, it's not permitted for one thread to remove another thread's
 * handlers.
 *
 * @section c_event_layeredPublishSubscribe Layered Publish-Subscribe Handlers
 *
 * If you need to implement an API that allows clients to register "handler"
 * functions to be called-back after a specific event occurs, the Event Loop API
 * provides some special help.
 *
 * You can have the Event Loop call your handler function (the first-layer handler),
 * to unpack specified items from the Event Report and call the client's handler function (the
 * second-layer handler).
 *
 * For example, you could create a "Temperature Sensor API" that allows its clients to register
 * handler functions to be called to handle changes in the temperature, like this:
 *
 * @code
 * // Temperature change handler functions must look like this.
 * typedef void (*tempSensor_ChangeHandlerFunc_t)(int32_t newTemperature, void* contextPtr);
 *
 * // Opaque type used to refer to a registered temperature change handler.
 * typedef struct tempSensor_ChangeHandler* tempSensor_ChangeHandlerRef_t;
 *
 * // Register a handler function to be called when the temperature changes.
 * tempSensor_ChangeHandlerRef_t tempSensor_AddChangeHandler
 * (
 *     tempSensor_ChangeHandlerFunc_t  handlerFunc,  // The handler function.
 *     void*                           contextPtr    // Opaque pointer to pass to handler function.
 * );
 *
 * // De-register a handler function that was previously registered using
 * // tempSensor_AddChangeHandler().
 * void tempSensor_RemoveChangeHandler
 * (
 *     tempSensor_ChangeHandlerRef_t  handlerRef
 * );
 * @endcode
 *
 * The implementation could look like this:
 *
 * @code
 * COMPONENT_INIT
 * {
 *     TempChangeEventId = le_event_CreateId("TempChange", sizeof(int32_t));
 * }
 *
 * static void TempChangeHandler
 * (
 *     void* reportPtr,
 *     void* secondLayerHandlerFunc
 * )
 * {
 *     int32_t* temperaturePtr = reportPtr;
 *     tempSensor_ChangeHandlerRef_t clientHandlerFunc = secondLayerHandlerFunc;
 *
 *     clientHandlerFunc(*temperaturePtr, le_event_GetContextPtr());
 * }
 *
 * tempSensor_ChangeHandlerRef_t tempSensor_AddChangeHandler
 * (
 *     tempSensor_ChangeHandlerFunc_t  handlerFunc,
 *     void*                           contextPtr
 * )
 * {
 *     le_event_HandlerRef_t handlerRef;
 *
 *     handlerRef = le_event_AddLayeredHandler("TempChange",
 *                                             TempChangeEventId,
 *                                             TempChangeHandler,
 *                                             handlerFunc);
 *     le_event_SetContextPtr(handlerRef, contextPtr);
 *
 *     return (tempSensor_ChangeHandlerRef_t)handlerRef;
 * }
 *
 * void tempSensor_RemoveChangeHandler
 * (
 *     tempSensor_ChangeHandlerRef_t    handlerRef
 * )
 * {
 *     le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
 * }
 * @endcode
 *
 *This approach gives strong type checking of both handler references and handler
 * function pointers in code that uses this Temperature Sensor API.
 *
 *
 * @section c_event_reportingRefCountedObjects Event Reports Containing Reference-Counted Objects
 *
 *Sometimes you need to report an event where the report payload is pointing to a
 * reference-counted object allocated from a memory pool (see @ref c_memory).
 * Memory leaks and/or crashes can result if its is sent through the Event Loop API
 * without telling the Event Loop API it's pointing to a reference counted object.
 * If there are no subscribers, the Event Loop API iscards the reference without releasing it,
 * and the object is never be deleted. If multiple handlers are registered,
 * the reference could be released by the handlers too many times. Also, there are
 * other, subtle issues that are nearly impossible to solve if threads terminate while
 * reports containing pointers to reference-counted objects are on their Event Queues.
 *
 * To help with this, the functions @c le_event_CreateIdWithRefCounting() and
 * @c le_event_ReportWithRefCounting() have been provided. These allow a pointer to a
 * reference-counted memory pool object to be sent as the payload of an Event Report.
 *
 * @c le_event_ReportWithRefCounting() passes ownership of one reference to the Event Loop API, and
 * when the handler is called, it receives ownership for one reference.  It then becomes
 * the handler's responsibility to release its reference (using le_mem_Release()) when it's done.
 *
 *
 * @c le_event_CreateIdWithRefCounting() is used the same way as le_event_CreateId(), except that
 * it doesn't require a payload size as the payload is always
 * known from the pointer to a reference-counted memory pool object.  Only Event IDs
 * created using le_event_CreateIdWithRefCounting() can be used with
 * le_event_ReportWithRefCounting().
 *
 * @code
 * static le_event_Id_t EventId;
 * le_mem_PoolRef_t MyObjectPoolRef;
 *
 * static void MyHandler
 * (
 *     void* reportPtr  // Pointer to my reference-counted object.
 * )
 * {
 *     MyObj_t* objPtr = reportPtr;
 *
 *     // Do something with the object.
 *     ...
 *
 *     // Okay, I'm done with the object now.
 *     le_mem_Release(objPtr);
 * }
 *
 * COMPONENT_INIT
 * {
 *     EventId = le_event_CreateIdWithRefCounting("SomethingHappened");
 *     le_event_AddHandler("SomethingHandler", EventId, MyHandler);
 *     MyObjectPoolRef = le_mem_CreatePool("MyObjects", sizeof(MyObj_t));
 * }
 *
 * static void ReportSomethingDetected
 * (
 *     ...
 * )
 * {
 *     MyObj_t* objPtr = le_mem_ForceAlloc(MyObjectPool);
 *
 *     // Fill in the object.
 *     ...
 *
 *     le_event_ReportWithRefCounting(EventId, objPtr);
 * }
 *
 * @endcode
 *
 * @section c_event_miscThreadingTopics Miscellaneous Multithreading Topics
 *
 * All functions in this API are thread safe.
 *
 * Each thread can have only one Event Loop.
 * The main thread in every Legato process will always run an Event Loop after it's run
 * the component initialization functions.  As soon as all component initialization functions
 * have returned, the main thread will start processing its event queue.
 *
 * When a function is called to "Add" an event handler, that handler is associated with the
 * calling thread's Event Loop. If the calling thread doesn't run its Event Loop, the event
 * reports will pile up in the queue, never getting serviced and never releasing their memory.
 * This will appear in the logs as event queue growth warnings.
 *
 * If a client starts its own thread (e.g., by calling le_thread_Create() ), then
 * that thread will @b not automatically run an Event Loop.   To make it run an Event Loop, it must
 * call @c le_event_RunLoop() (which will never return).
 *
 * If a thread running an Event Loop terminates, the Legato framework automatically
 * deregisters any handlers and deletes the thread's Event Loop, its Event
 * Queue, and any event reports still in that Event Queue.
 *
 * @section c_event_integratingLegacyPosix Integrating with Legacy POSIX Code
 *
 * Many legacy programs written on top of POSIX APIs will have previously built their own event loop
 * using poll(), select(), or some other blocking functions. It may be difficult to refactor this type of
 * event loop to use the Legato event loop instead.
 *
 * Two functions are provided to assist integrating legacy code with the Legato
 * Event Loop:
 *  - @c le_event_GetFd() - Fetches a file descriptor that can be monitored using some variant of
 *                       poll() or select() (including epoll).  It will appear readable when the
 *                       Event Loop needs servicing.
 *  - @c le_event_ServiceLoop() - Services the event loop.  This should be called if the file
 *                       descriptor returned by le_event_GetFd() appears readable to poll() or
 *                       select().
 *
 * In an attempt to avoid starving the caller when there are a lot of things that need servicing
 * on the Event Loop, @c le_event_ServiceLoop() will only perform one servicing step (i.e., call one
 * event handler function) before returning, regardless of how much work there is to do.  It's
 * the caller's responsibility to check the return code from le_event_ServiceLoop() and keep
 * calling until it indicates that there is no more work to be done.
 *
 * @section c_event_troubleshooting Troubleshooting
 *
 * A logging keyword can be enabled to view a given thread's event handling activity.  The keyword name
 * depends on the thread and process name where the thread is located.
 * For example, the keyword "P/T/events" controls logging for a thread named "T" running inside
 * a process named "P".
 *
 * @todo Add a reference to the Process Inspector and its capabilities for inspecting Event Queues,
 * Event Loops, Handlers and Event Report statistics.

 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/** @file le_eventLoop.h
 *
 * Legato @ref c_eventLoop include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_EVENTLOOP_INCLUDE_GUARD
#define LEGATO_EVENTLOOP_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Event ID.
 *
 * An Event ID ties event reports to event handlers.  See @ref c_event_publishSubscribe for
 * more details.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_event_Id* le_event_Id_t;


//--------------------------------------------------------------------------------------------------
/**
 * Initialization event handler function declaration macro.
 *
 * Use this macro instead of a normal function prototype to create an Initialization Event Handler
 * function.  E.g.,
 *
 * @code
 * COMPONENT_INIT
 * {
 *     // Do my initialization here...
 * }
 * @endcode
 *
 * @return Nothing.
 */
//--------------------------------------------------------------------------------------------------

#ifdef __cplusplus
    #define LE_CI_LINKAGE extern "C"
#else
    #define LE_CI_LINKAGE
#endif

// This macro is set by the build system.  However, if it hasn't been set, use a sensible default.
// TODO: Remove this.
#ifndef COMPONENT_INIT
    /**
     * Initialization event handler function.
     */
    #define COMPONENT_INIT LE_CI_LINKAGE LE_SHARED void _le_event_InitializeComponent(void)
#endif

/// Deprecated name for @ref COMPONENT_INIT.
#define LE_EVENT_INIT_HANDLER COMPONENT_INIT


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for publish-subscribe event handler functions look like this:
 *
 * @param reportPtr [in] Pointer to the event report payload.
 *
 * @warning The reportPtr is only valid until the handler function returns.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_event_HandlerFunc_t)
(
    void* reportPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for the first layer of a layered publish-subscribe event handler function
 * look like this:
 *
 * @param reportPtr [in] Pointer to the event report payload.
 *
 * @param secondLayerFunc [in] Address of the second layer handler function.
 *
 * @warning The reportPtr is only valid until the handler function returns.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_event_LayeredHandlerFunc_t)
(
    void*   reportPtr,
    void*   secondLayerFunc
);


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for deferred functions look like this:
 * @param param1Ptr [in] Value passed in as param1Ptr to le_event_QueueFunction().
 * @param param2Ptr [in] Value passed in as param2Ptr to le_event_QueueFunction().
 *
 * See @ref c_event_deferredFunctionCalls for more information.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_event_DeferredFunc_t)
(
    void* param1Ptr,
    void* param2Ptr
);


//--------------------------------------------------------------------------------------------------
/**
 * Handler reference.
 *
 *Used to refer to handlers that have been added for events. Only needed if
 * you want to set the handler's context pointer or need to remove the handler later.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_event_Handler* le_event_HandlerRef_t;


#if LE_CONFIG_EVENT_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/**
 * Create a new event ID.
 *
 *  @param[in]  name        Name of the event ID.  (Named for diagnostic purposes.)
 *  @param[in]  payloadSize Data payload size (in bytes) of the event reports (can be 0).
 *
 *  @return Event ID.
 *
 *  @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t le_event_CreateId
(
    const char *name,
    size_t      payloadSize
);
#else /* if not LE_CONFIG_EVENT_NAMES_ENABLED */
/// @cond HIDDEN_IN_USER_DOCS
//--------------------------------------------------------------------------------------------------
/**
 * Internal function used to implement le_event_CreateId().
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t _le_event_CreateId(size_t payloadSize);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Create a new event ID.
 *
 *  @param[in]  name        Name of the event ID.  (Named for diagnostic purposes.)
 *  @param[in]  payloadSize Data payload size (in bytes) of the event reports (can be 0).
 *
 *  @return Event ID.
 *
 *  @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE le_event_Id_t le_event_CreateId
(
    const char *name,
    size_t      payloadSize
)
{
    LE_UNUSED(name);
    return _le_event_CreateId(payloadSize);
}
#endif /* end LE_CONFIG_EVENT_NAMES_ENABLED */


#if LE_CONFIG_EVENT_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/**
 * Create a new event ID to report events where the payload is a pointer to
 * a reference-counted memory pool object allocated using the @ref c_memory.
 *
 *  @param[in]  name    Name of the event ID.  (Named for diagnostic purposes.)
 *
 *  @return Event ID.
 *
 *  @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t le_event_CreateIdWithRefCounting
(
    const char *name
);
#else /* if not LE_CONFIG_EVENT_NAMES_ENABLED */
/// @cond HIDDEN_IN_USER_DOCS
//--------------------------------------------------------------------------------------------------
/**
 * Internal function used to implement le_event_CreateIdWithRefCounting().
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t _le_event_CreateIdWithRefCounting(void);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Create a new event ID to report events where the payload is a pointer to
 * a reference-counted memory pool object allocated using the @ref c_memory.
 *
 *  @param[in]  name    Name of the event ID.  (Named for diagnostic purposes.)
 *
 *  @return Event ID.
 *
 *  @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE le_event_Id_t le_event_CreateIdWithRefCounting
(
    const char *name
)
{
    LE_UNUSED(name);
    return _le_event_CreateIdWithRefCounting();
}
#endif /* end LE_CONFIG_EVENT_NAMES_ENABLED */


#if LE_CONFIG_EVENT_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/**
 * Adds a handler function for a publish-subscribe event ID.
 *
 * Tells the calling thread event loop to call a specified handler function when a defined event
 * reaches the front of the event queue.
 *
 *  @param[in]  name        Handler name.
 *  @param[in]  eventId     Event ID.
 *  @param[in]  handlerFunc Handler function.
 *
 *  @return
 *      Handler reference, only needed to remove the handler (using
 *      le_event_RemoveHandler() ).  Can be ignored if the handler will never be removed.
 *
 *  @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t le_event_AddHandler
(
    const char              *name,
    le_event_Id_t            eventId,
    le_event_HandlerFunc_t   handlerFunc
);
#else /* if not LE_CONFIG_EVENT_NAMES_ENABLED */
/// @cond HIDDEN_IN_USER_DOCS
//--------------------------------------------------------------------------------------------------
/**
 * Internal function used to implement le_event_AddHandler().
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t _le_event_AddHandler(le_event_Id_t eventId, le_event_HandlerFunc_t handlerFunc);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Adds a handler function for a publish-subscribe event ID.
 *
 * Tells the calling thread event loop to call a specified handler function when a defined event
 * reaches the front of the event queue.
 *
 *  @param[in]  name        Handler name.
 *  @param[in]  eventId     Event ID.
 *  @param[in]  handlerFunc Handler function.
 *
 *  @return
 *      Handler reference, only needed to remove the handler (using
 *      le_event_RemoveHandler() ).  Can be ignored if the handler will never be removed.
 *
 *  @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE le_event_HandlerRef_t le_event_AddHandler
(
    const char              *name,
    le_event_Id_t            eventId,
    le_event_HandlerFunc_t   handlerFunc
)
{
    LE_UNUSED(name);
    return _le_event_AddHandler(eventId, handlerFunc);
}
#endif /* end LE_CONFIG_EVENT_NAMES_ENABLED */


#if LE_CONFIG_EVENT_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/**
 * Adds a layered handler function for a publish-subscribe event ID.
 *
 * Tells the calling thread event loop to call a specified handler function when a defined event
 * reaches the front of the event queue.  Passes the required handler functions when called.
 *
 * This is intended for use in implementing @ref c_event_layeredPublishSubscribe.
 *
 *  @param[in]  name    Handler name.
 *  @param[in]  name    Event ID.
 *  @param[in]  name    Pointer to first-layer handler func.
 *  @param[in]  name    Pointer to second-layer handler func.
 *
 *  @return
 *      Handler reference, only needed for later removal of the handler (using
 *      le_event_RemoveHandler() ).  Can be ignored if the handler will never be removed.
 *
 *  @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t le_event_AddLayeredHandler
(
    const char                      *name,
    le_event_Id_t                    eventId,
    le_event_LayeredHandlerFunc_t    firstLayerFunc,
    void*                            secondLayerFunc
);
#else /* if not LE_CONFIG_EVENT_NAMES_ENABLED */
/// @cond HIDDEN_IN_USER_DOCS
//--------------------------------------------------------------------------------------------------
/**
 * Internal function used to implement le_event_AddLayeredHandler().
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t _le_event_AddLayeredHandler(le_event_Id_t eventId,
                                                 le_event_LayeredHandlerFunc_t firstLayerFunc,
                                                 void* secondLayerFunc);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Adds a layered handler function for a publish-subscribe event ID.
 *
 * Tells the calling thread event loop to call a specified handler function when a defined event
 * reaches the front of the event queue.  Passes the required handler functions when called.
 *
 * This is intended for use in implementing @ref c_event_layeredPublishSubscribe.
 *
 *  @param[in]  name    Handler name.
 *  @param[in]  name    Event ID.
 *  @param[in]  name    Pointer to first-layer handler func.
 *  @param[in]  name    Pointer to second-layer handler func.
 *
 *  @return
 *      Handler reference, only needed for later removal of the handler (using
 *      le_event_RemoveHandler() ).  Can be ignored if the handler will never be removed.
 *
 *  @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE le_event_HandlerRef_t le_event_AddLayeredHandler
(
    const char                      *name,
    le_event_Id_t                    eventId,
    le_event_LayeredHandlerFunc_t    firstLayerFunc,
    void*                            secondLayerFunc
)
{
    LE_UNUSED(name);
    return _le_event_AddLayeredHandler(eventId, firstLayerFunc, secondLayerFunc);
}
#endif /* end LE_CONFIG_EVENT_NAMES_ENABLED */


//--------------------------------------------------------------------------------------------------
/**
 * Remove Handler
 *
 * Removes a previously added event handler function.
 */
//--------------------------------------------------------------------------------------------------
void le_event_RemoveHandler
(
    le_event_HandlerRef_t   handlerRef  ///< [in] Handler reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Report an Event
 *
 * Queues an Event Report to any and all event loops that have handlers for that event.
 *
 * @note Copies the event report payload, so it is safe to release or reuse the buffer that
 *       payloadPtr points to as soon as le_event_Report() returns.
 */
//--------------------------------------------------------------------------------------------------
void le_event_Report
(
    le_event_Id_t   eventId,    ///< [in] Event ID created using le_event_CreateId().
    void*           payloadPtr, ///< [in] Pointer to the payload bytes to be copied into the report.
    size_t          payloadSize ///< [in] Number of bytes of payload to copy into the report.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sends an Event Report with a pointer to a reference-counted object as its payload.
 * The pointer must have been obtained from a memory pool using the @ref c_memory.
 *
 * Calling this function passes ownership of the reference to the Event Loop API.
 * The Event Loop API will ensure that the reference is properly counted while it passes
 * through the event report dispatching system.  Each handler will receive one counted reference
 * to the object, so the handler is responsible for releasing the object when it is finished with
 * it.
 */
//--------------------------------------------------------------------------------------------------
void le_event_ReportWithRefCounting
(
    le_event_Id_t   eventId,    ///< [in] Event ID created using le_event_CreateIdWithRefCounting().
    void*           objectPtr   ///< [in] Pointer to an object allocated from a memory pool
                                ///       (using the @ref c_memory).
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the context pointer for a given event handler.
 *
 * This can later be retrieved using le_event_GetContextPtr() from within the handler function
 * when it is called.
 */
//--------------------------------------------------------------------------------------------------
void le_event_SetContextPtr
(
    le_event_HandlerRef_t   handlerRef, ///< [in] Handler where context pointer is to be set.
    void*                   contextPtr  ///< [in] Context pointer value.
);


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the context pointer for the currently running event handler.
 *
 * Can only be called from within an event handler function.
 *
 * @return
 *      Context pointer that was set using le_event_SetContextPtr(), or NULL if
 *      le_event_SetContextPtr() was not called.
 */
//--------------------------------------------------------------------------------------------------
void* le_event_GetContextPtr
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Queue a function onto the calling thread's Event Queue.  When it reaches the head of the
 * Event Queue, it will be called by the calling thread's Event Loop.
 */
//--------------------------------------------------------------------------------------------------
void le_event_QueueFunction
(
    le_event_DeferredFunc_t func,       ///< [in] Function to be called later.
    void*                   param1Ptr,  ///< [in] Value to be passed to the function when called.
    void*                   param2Ptr   ///< [in] Value to be passed to the function when called.
);


//--------------------------------------------------------------------------------------------------
/**
 * Queue a function onto a specific thread's Event Queue.  When it reaches the head of that
 * Event Queue, it will be called by that thread's Event Loop.
 */
//--------------------------------------------------------------------------------------------------
void le_event_QueueFunctionToThread
(
    le_thread_Ref_t         thread,     ///< [in] Thread to queue the function to.
    le_event_DeferredFunc_t func,       ///< [in] The function.
    void*                   param1Ptr,  ///< [in] Value to be passed to the function when called.
    void*                   param2Ptr   ///< [in] Value to be passed to the function when called.
);


//--------------------------------------------------------------------------------------------------
/**
 * Runs the event loop for the calling thread.
 *
 * This starts processing events by the calling thread.
 *
 * Can only be called once for each thread, and must never be called in
 * the process's main thread.
 *
 * @note
 *      Function never returns.
 */
//--------------------------------------------------------------------------------------------------
void le_event_RunLoop
(
    void
)
__attribute__ ((noreturn));


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a file descriptor that will appear readable to poll(), select(), epoll_wait(), etc.
 * when the calling thread's Event Loop needs servicing (via a call to le_event_ServiceLoop()).
 *
 * @warning Only intended for use when integrating with legacy POSIX-based software
 * that cannot be easily refactored to use the Legato Event Loop.  The preferred approach is
 * to call le_event_RunLoop().
 *
 * @return The file descriptor.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API int le_event_GetFd
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Services the calling thread's Event Loop.
 *
 * @warning Only intended for use when integrating with legacy POSIX-based software
 * that can't be easily refactored to use the Legato Event Loop.  The preferred approach is
 * to call le_event_RunLoop().
 *
 * See also: le_event_GetFd().
 *
 * @return
 *  - LE_OK if there is more to be done.  DO NOT GO BACK TO SLEEP without calling
 *          le_event_ServiceLoop() again.
 *  - LE_WOULD_BLOCK if there is nothing left to do for now and it is safe to go back to sleep.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_result_t le_event_ServiceLoop
(
    void
);


#endif // LEGATO_EVENTLOOP_INCLUDE_GUARD
