/**
 * @page c_eventLoop Event Loop API
 *
 * @ref le_eventLoop.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 *  @ref c_event_deferredFunctionCalls <br>
 *  @ref c_event_publishSubscribe <br>
 *  @ref c_event_layeredPublishSubscribe <br>
 *  @ref c_event_files <br>
 *  @ref c_event_dispatchingToOtherThreads <br>
 *  @ref c_event_miscThreadingTopics <br>
 *  @ref c_event_troubleshooting <br>
 *  @ref c_event_integratingLegacyPosix <br>

 * The Event Loop API supports Legato's @ref programmingModel.  In this event-driven
 * programming model, a central <b> event loop </b> calls <b> event handler </b> functions in
 * response to <b>event reports</b>.
 *
 * Software components register their event handler functions with the event system (either
 * directly through the Event Loop API or indirectly through other APIs that use the Event Loop API)
 * so the central event loop knows the functions to call in response to defined events.
 *
 * Every event loop has an <b>event queue</b>, which is a queue of events waiting to be handled by that event loop.
 * The following different usage patterns are supported by the Event Loop API:
 *
 * @note When the process dies, all events, event loops, queues, reports, and handlers will be
 * automatically cleared.
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
 * @section c_event_files Working with File Descriptors
 *
 * In a POSIX environment, like Linux, file descriptors are used for most process I/O.
 * Many components will need to be notified when one or more file descriptors are
 * ready to read from or write to, or experience an error or hang-up.
 *
 * In conventional programs, it's common to block a thread on a call to @c read(), @c write(),
 * @c accept(), @c select(), @c poll(), or some variant of those functions.
 * But if that's done in a thread shared with other components, those other
 * components would be unable to run when needed.  To avoid this, the Legato
 * event system provides methods to monitor file descriptors and report related events so they
 * won't interfere with other software sharing the same thread.
 *
 * To start monitoring a file descriptor:
 * -# a <b> File Descriptor Monitor </b> object is created for that file descriptor (by calling
 *    le_event_CreateFdMonitor() ) and
 * -# event handler functions are registered with it (by calling le_event_SetFdHandler()).
 * -# optionally, make the event deferrable until next system resume (by calling le_event_WakeUp()
 *    with wakeUp flag set to 'false').
 *
 * File descriptor event handler functions receive a file descriptor as their only parameter,
 * instead of receiving a report pointer.  See @ref le_event_FdEventType_t for a list of events
 * that can be handled for file descriptors.
 *
 * For example:
 *
 * @code

COMPONENT_INIT
{
    // Open the serial port.
    int fd = open("/dev/ttyS0", O_RDWR|O_NONBLOCK);
    LE_FATAL_IF(fd == -1, "open failed with errno %d (%m)", errno);

    // Create a File Descriptor Monitor object for the serial port's file descriptor.
    le_event_FdMonitorRef_t fdMonitor = le_event_CreateFdMonitor("PortMonitor", fd);

    // Register a read handler (note: context pointer is unused in this example).
    le_event_SetFdHandler(fdMonitor, LE_EVENT_FD_READABLE, MyReadHandler);
}


static void MyReadHandler(int fd)
{
    char buff[MY_BUFF_SIZE];

    ssize_t bytesRead = read(fd, buff, sizeof(buff));

    ...
}

 * @endcode
 *
 * If an event occurs on a file descriptor where there is no handler for that event on that file descriptor,
 * the event will be ignored. If a handler is later registered for that event, and that event's
 * trigger condition is still true (e.g., the file descriptor still has data available to be read), then the
 * event will be reported to the handler at that time. If the event trigger condition is gone
 *  (e.g., the file descriptor no longer has data available to read), then the event will not be reported
 * until its trigger condition becomes true again.
 *
 * If events occur on different file descriptors at the same time, the order in which the handlers
 * are called is implementation-dependent. If multiple events occur on the same file descriptor at the same
 * time, handlers will be called in the same order as the events appear in the
 * @ref le_event_FdEventType_t enumeration (first event's handler will be called first). For
 * example, if data arrives and the far end closes the connection, the "readable" event handler
 * would be called before the "read hang up" event handler.
 *
 * When a file descriptor no longer needs to be monitored, the File Descriptor Monitor object
 * is deleted by calling le_event_DeleteFdMonitor().  There's no need to remove its handlers first.
 *
 * @warning Depending on the implementation, strange behaviour may occur if a file descriptor is
 * closed while being monitored and then the same file descriptor is reused for something else
 * before its Monitor object is deleted. Always delete the Monitor object for a file descriptor
 * when it is closed.
 *
 * A file descriptor event handler can be removed (deregistered) using @c le_event_ClearFdHandler()
 * or le_event_ClearFdHandlerByEventType().
 * This is useful monitor writeability.  When the file descriptor is writeable,
 * but there's nothing to write, the writeability handler will be continuously run
 * until it's cleared or enough data is written into the file descriptor to cause it to become
 * unwriteable.
 * Allowing the handler to continually run is a colossal waste of CPU cycles
 * and power. To prevent this, clear the writeability handler and set it again later when an
 * attempt to write is rejected because the file descriptor is no longer writeable.
 *
 * @code
static void DoWrite()
{
    le_result_t result = WriteMoreStuff();
    if (result == LE_WOULD_BLOCK)
    {
        // The connection is not writeable (because its send buffers are full).
        // Register for notification when it becomes writeable again.
        FdWriteableHandlerRef = le_event_SetFdHandler(FdMonitorRef,
                                                      LE_EVENT_FD_WRITEABLE,
                                                      ContinueWriting);
    }
    else
    {
        ...
    }
}

static void ContinueWriting(int fd)
{
    le_event_ClearFdHandler(FdWriteableHandlerRef);

    DoWrite();
}
 * @endcode
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
 * Monitoring a file descriptor is performed by the Event Loop of the thread that
 * created the Monitor object for that file descriptor.  If that thread is blocked, no events
 * will be detected for that file descriptor until that thread is unblocked and returns to its
 * Event Loop.  Likewise, if the thread that creates a File Descriptor Monitor object does not
 * run an Event Loop at all, no events will be detected for that file descriptor.
 *
 * It's not recommended to use the same file descriptor to monitor two threads at the same time.
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
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

//--------------------------------------------------------------------------------------------------
/** @file le_eventLoop.h
 *
 * Legato @ref c_eventLoop include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
#ifndef COMPONENT_INIT
    #define COMPONENT_INIT LE_CI_LINKAGE void _le_event_InitializeComponent(void)
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
 * Prototype for file descriptor event handler functions look like this:
 *
 * @param fd    [in] File descriptor that experienced the event.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_event_FdHandlerFunc_t)
(
    int fd
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
 * File Descriptor Monitor reference.
 *
 * Used to refer to File Descriptor Monitor objects.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_event_FdMonitor* le_event_FdMonitorRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Handler reference.
 *
 *Used to refer to handlers that have been added for events. Only needed if
 * you want to set the handler's context pointer or need to remove the handler later.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_event_Handler* le_event_HandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * File Descriptor Handler reference.
 *
 * Used to refer to handlers that have been set for File Descriptor events.  Only needed if
 * you need to set te handler's context pointer or remove the handler later.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_event_FdHandler* le_event_FdHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Enumerates all the different types of events that can be generated for a file descriptor.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_EVENT_FD_READABLE,           ///< Data is available for reading.
    LE_EVENT_FD_READABLE_URGENT,    ///< Urgent/out-of-band data is available for reading.
    LE_EVENT_FD_WRITEABLE,          ///< Ready to accept data for writing.
    LE_EVENT_FD_WRITE_HANG_UP,      ///< Far end shutdown their reading while we were still writing.
    LE_EVENT_FD_READ_HANG_UP,       ///< Far end shutdown their writing while we were still reading.
    LE_EVENT_FD_ERROR,              ///< Experienced an error.
}
le_event_FdEventType_t;

#define LE_EVENT_NUM_FD_EVENT_TYPES 6   ///< Number of members in the le_event_FdEventType_t enum.


//--------------------------------------------------------------------------------------------------
/**
 * Create a new event ID.
 *
 * @return
 *      Event ID.
 *
 * @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t le_event_CreateId
(
    const char* name,       ///< [in] Name of the event ID.  (Named for diagnostic purposes.)
    size_t      payloadSize ///< [in] Data payload size (in bytes) of the event reports (can be 0).
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a new event ID to report events where the payload is a pointer to
 * a reference-counted memory pool object allocated using the @ref c_memory.
 *
 * @return
 *      Event ID.
 *
 * @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t le_event_CreateIdWithRefCounting
(
    const char* name        ///< [in] Name of the event ID.  (Named for diagnostic purposes.)
);


//--------------------------------------------------------------------------------------------------
/**
 * Adds a handler function for a publish-subscribe event ID.
 *
 * Tells the calling thread event loop to call a specified handler function when a defined event
 * reaches the front of the event queue.
 *
 * @return
 *      Handler reference, only needed to remove the handler (using
 *      le_event_RemoveHandler() ).  Can be ignored if the handler will never be removed.
 *
 * @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t le_event_AddHandler
(
    const char*             name,           ///< [in] Handler name.
    le_event_Id_t           eventId,        ///< [in] Event ID.
    le_event_HandlerFunc_t  handlerFunc     ///< [in] Handler function.
);


//--------------------------------------------------------------------------------------------------
/**
 * Adds a layered handler function for a publish-subscribe event ID.
 *
 * Tells the calling thread event loop to call a specified handler function when a defined event
 * reaches the front of the event queue.  Passes the required handler functions when called.
 *
 * This is intended for use in implementing @ref c_event_layeredPublishSubscribe.
 *
 * @return
 *      Handler reference, only needed for later removal of the handler (using
 *      le_event_RemoveHandler() ).  Can be ignored if the handler will never be removed.
 *
 * @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t le_event_AddLayeredHandler
(
    const char*                     name,           ///< [in] Handler name.
    le_event_Id_t                   eventId,        ///< [in] Event ID.
    le_event_LayeredHandlerFunc_t   firstLayerFunc, ///< [in] Pointer to first-layer handler func.
    void*                           secondLayerFunc ///< [in] Pointer to second-layer handler func.
);


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
 * Creates a File Descriptor Monitor.
 *
 * Creates an object that will monitor a given file descriptor for events.
 *
 * The monitoring will be performed by the event loop of the thread that created the Monitor object.
 * If that thread is blocked, no events will be detected for that file descriptor until that
 * thread is unblocked and returns to its event loop.
 *
 * @return
 *      Reference to the object, which is needed for later deletion.
 *
 * @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_FdMonitorRef_t le_event_CreateFdMonitor
(
    const char*             name,       ///< [in] Name of the object (for diagnostics).
    int                     fd          ///< [in] File descriptor to be monitored for events.
);


//--------------------------------------------------------------------------------------------------
/**
 * Registers a handler for a specific type of file descriptor event with a given
 * File Descriptor Monitor object.
 *
 * When the handler function is called, it will be called by the the thread that registered
 * the handler, which must also be the same thread that created the FD Monitor object.
 *
 * @return  Reference to the handler function. (Only needed if the handler's context pointer
 *          needs to be set.)
 *
 * @note
 * - Doesn't return on failure, there's no need to check the return value for errors.
 * - The only way to deregister an FD Monitor event handler is to delete the FD Monitor object.
 *   Otherwise, there can be races between event reports and handler registration/deregistration
 *   that could result in spurious handler function calls.
 */
//--------------------------------------------------------------------------------------------------
le_event_FdHandlerRef_t le_event_SetFdHandler
(
    le_event_FdMonitorRef_t  monitorRef, ///< [in] Reference to the File Descriptor Monitor object.
    le_event_FdEventType_t   eventType,  ///< [in] Type of event to be reported to this handler.
    le_event_FdHandlerFunc_t handlerFunc ///< [in] Handler function.
);


//--------------------------------------------------------------------------------------------------
/**
 * Indicate if the event is deferrable or system should stay awake while processing the event from
 * File Descriptor Monitor object. Components wanting to take advantage of this feature have to be
 * assigned CAP_EPOLLWAKEUP (or CAP_BLOCK_SUSPEND) capability.
 */
//--------------------------------------------------------------------------------------------------
void le_event_WakeUp
(
    le_event_FdMonitorRef_t monitorRef, ///< [in] Reference to the File Descriptor Monitor object.
    bool                    wakeUp      ///< [in] true (wake up) or false (deferred, no wake up).
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the Context Pointer for a handler for a file descriptor event.  This can be retrieved
 * by the handler by calling le_event_GetContextPtr() when the handler function is running.
 */
//--------------------------------------------------------------------------------------------------
void le_event_SetFdHandlerContextPtr
(
    le_event_FdHandlerRef_t handlerRef, ///< [in] Reference to the handler.
    void*                   contextPtr  ///< [in] Opaque context pointer value.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deregisters a handler for a file descriptor event.
 */
//--------------------------------------------------------------------------------------------------
void le_event_ClearFdHandler
(
    le_event_FdHandlerRef_t  handlerRef  ///< [in] Reference to the handler.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deregisters a handler for a file descriptor event.
 */
//--------------------------------------------------------------------------------------------------
void le_event_ClearFdHandlerByEventType
(
    le_event_FdMonitorRef_t  monitorRef, ///< [in] Reference to the File Descriptor Monitor object.
    le_event_FdEventType_t   eventType   ///< [in] The type of event to clear the handler for.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a file descriptor monitor object.
 *
 * This will automatically remove all handlers added to the object.
 */
//--------------------------------------------------------------------------------------------------
void le_event_DeleteFdMonitor
(
    le_event_FdMonitorRef_t monitorRef  ///< [in] Reference to the File Descriptor Monitor object.
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
int le_event_GetFd
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
le_result_t le_event_ServiceLoop
(
    void
);


#endif // LEGATO_EVENTLOOP_INCLUDE_GUARD
